#pragma once
#include "db_conn_pool.hpp"
#include "epoller.hpp"
#include "sock.hpp"
#include <unordered_map>
#include <grpc++/grpc++.h>
#include "connection.hpp"
#include "thread_pool.hh"
#include <functional>
#include "protocol.hpp"
#include "minilog.hh"
#include "service.h"
#include <unordered_set>
namespace tinychat{
    constexpr uint16_t default_port = 55369;
    
    class EpollServer
    {
    public:
        const int lsnfd;
        DBConnPool<7> dbconns;
        ThreadPool<7> workers;
        
        // aka connections, key: a connection's fd
        std::unordered_map<int,std::unique_ptr<tinychat::Connection>> conns_;
        Epoller epoller_; // epoll
        
        std::unordered_map<int, std::unordered_set<int>> gid_to_users_;

        EpollServer() : lsnfd(socket(AF_INET, SOCK_STREAM, 0))
        {
            // log_debug("lsnfd: {}",lsnfd);

        }
        ~EpollServer()
        {
            // lsnsock_.cl();
        }

        void boot()
        {
            auto lsnsock = std::make_unique<tinychat::Sock>(lsnfd, default_port);
            lsnsock->bind();
            lsnsock->listen();
            auto lsn = std::make_unique<tinychat::Connection>(std::move(lsnsock), this);
            lsn->register_callback(std::bind(&EpollServer::accepter, this, std::placeholders::_1), nullptr, nullptr);
            add_conn(std::move(lsn), EPOLLIN | EPOLLET);
            auto dbconn = dbconns.get();
            std::string query_groups = "select * from Groups;";
            auto query = dbconn->query(query_groups);
            auto result = query.store();
            if (result.num_rows() > 0)
            {
                for (int i = 0; i < result.num_rows(); ++i)
                {
                    gid_to_users_.insert({std::stoi(std::string(result[i][0])), std::unordered_set<int>()});
                    log_debug("{}", std::stoi(std::string(result[i][0])));
                }
            }
            while (true)
            {
                int timeout = 1000;
                int n = epoller_.wait(timeout);
                switch (n)
                {
                    case -1:
                        perror("epoll_wait");
                        break;
                    case 0:
                        // cout << "timeout\n";
                        break;
                    default:
                        dispatcher(n);
                        break;
                }
            }
        }
        void shutdown()
        {
            for (auto &[k, v] : conns_)
            {
                epoller_.delete_event(k);
                v.reset();
            }
            std:: cout << "server shutdown\n";
        }
        // dispatch events to thread pool or accept incoming connections
        void dispatcher(int event_cnt)
        {
            // cout << "event_cnt: " << event_cnt << endl;
            for (int i = 0; i < event_cnt; ++i)
            {
                auto event = epoller_.events_[i];
                int fd = event.data.fd;
                auto type = event.events;
                // //log_debug("get event");
                if (type & EPOLLERR || type & EPOLLHUP)
                    type |= (EPOLLIN | EPOLLOUT);
                if (type & EPOLLIN)
                    conns_[fd]->receiver_(conns_[fd]);
                if (type & EPOLLOUT)
                    conns_[fd]->sender_(conns_[fd]);
            }
        }
        // add a connection to epoller and conns, btw you should set the epoll events that this connection cares
        void add_conn(std::unique_ptr<tinychat::Connection> conn, uint32_t event)
        {
            if (event & EPOLLET)
                fcntl(conn->client_->fd(), F_SETFD, F_GETFL | O_NONBLOCK);
            epoller_.add_event(conn->client_->fd(), event);
            conns_.emplace(conn->client_->fd(), std::move(conn));
            conn->outbuf_=std::to_string(conn->client_->fd());
            sender(conn);
        }
        
        // listen socket's reader, register a client's connection to epoller and conns
        void accepter(std::unique_ptr<tinychat::Connection> &conn)
        {
            do
            {
                auto clientconn = std::make_unique<tinychat::Connection>(conn->client_->accept(), this);
                clientconn->register_callback(std::bind(&EpollServer::receiver, this, std::placeholders::_1),
                                              std::bind(&EpollServer::sender, this, std::placeholders::_1),
                                              std::bind(&EpollServer::excepter, this, std::placeholders::_1));
                add_conn(std::move(clientconn), EPOLLIN | EPOLLET);
            } while (conn->event_ & EPOLLET);
            
        }
        
        // aka reader, read bytes from a connection's buffer and parse it into a response if possible
        void receiver(std::unique_ptr<tinychat::Connection> &conn)
        {
            // log_debug("receive ok");
            char buf[bufsize];
            do
            {
                bzero(buf, sizeof(buf));
                auto n = recv(conn->client_->fd(), buf, sizeof(buf) - 1, 0);
                if (n > 0)
                {
                    // buf[n-1]=0;buf[n-2]=0;
                    conn->inbuf_ += buf;
                    // log_debug("now inbuf is : |{}|,length:{}", conn->inbuf_, connect->inbuf_.size());
                    tinychat::Chat incomingMsg;
                    incomingMsg.ParseFromString(conn->inbuf_);
                    handle_postmsg(conn,incomingMsg);
                }
                else
                {
                    conn->excepter_(conn);
                    return;
                }
                
            } while (conn->event_ & EPOLLET);
        }
        
        // send to a connection's output buffer, you should always make sure that you sent a valid response's serialization
        void sender(std::unique_ptr<tinychat::Connection> &conn)
        {
            // //log_debug("sender triggered");
            do
            {
                auto n = send(conn->client_->fd(), conn->outbuf_.c_str(), conn->outbuf_.size(), 0);
                // log_debug("sendding n={}", n);
                if (n > 0)
                {
                    conn->outbuf_.erase(0, n);
                }
                else
                {
                    if (errno == EAGAIN || errno == EWOULDBLOCK)
                        break;
                    else if (errno == EINTR)
                        continue;
                    else
                    {
                        perror("send");
                        // be careful, excepter will destroy your connection and sock, so it should return instead of break
                        conn->excepter_(conn);
                        return;
                    }
                }
            } while (conn->event_ & EPOLLET);
            if (conn->outbuf_.empty())
            {
                epoller_.rwcfg(conn, true, false);
            }
            else
            {
                epoller_.rwcfg(conn, true, true);
            }
        }
        // if a connection is in exception, you should use this to shut down the connection
        void excepter(std::unique_ptr<tinychat::Connection> &conn)
        {
            log_info("exception occured, closing : fd:{} ip:{} port:{}", conn->client_->fd(), conn->client_->ip(), conn->client_->port());
            epoller_.delete_event(conn->client_->fd());
            for (auto &[gid, users] : gid_to_users_)
            {
                if (users.contains(conn->client_->fd()))
                {
                    users.erase(conn->client_->fd());
                    break;
                }
            }
            conns_.erase(conn->client_->fd());
        }
        mysqlpp::Connection *get()
        {
            return dbconns.get();
        }
        void ret(mysqlpp::Connection *conn)
        {
            dbconns.ret(conn);
        }
    };

}