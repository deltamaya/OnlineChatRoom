#include "protocol.hpp"
#include "utils.h"
#include <thread>
#include "minilog.hh"
#include <mysql++/mysql++.h>
#include "sock.hpp"
#include "connection.hpp"
#include "thread_pool.hh"
#include "epoller.hpp"
using namespace std;
int handle_signup(const Response &res);
int handle_msg(unique_ptr<Connection> &conn, const Response &res);
int handle_login(const Response &res);
const string serveraddr = "127.0.0.1";
const uint16_t port = 55369;
string userid, pwd, username;
ThreadPool<5> workers;
unordered_map<int, string> uid_to_name;
void sender(unique_ptr<Connection> &conn)
{
    while (!conn->outbuf_.empty())
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
    }
}
future<int> receiver(unique_ptr<Connection> &conn)
{
    future<int> ret;
    char buf[bufsize];
    while (1)
    {
        bzero(buf, sizeof(buf));
        // log_debug("trying to read data from server");
        auto n = recv(conn->client_->fd(), buf, sizeof(buf) - 1, 0);
        if (n > 0)
        {
            // buf[n-1]=0;buf[n-2]=0;
            conn->inbuf_ += buf;
            // log_debug("now inbuf is : |{}|,length:{}", conn->inbuf_, conn->inbuf_.size());
            Response res;
            bool ok = Response::parse_response(conn->inbuf_, &res);
            if (ok)
            {
                // log_debug("parse ok");
                switch (res.servcode_)
                {
                case ServiceCode::login:
                    ret = workers.submit(handle_login, res);
                    break;
                case ServiceCode::signup:
                    ret = workers.submit(handle_signup, res);
                    break;
                case ServiceCode::postmsg:
                    ret = workers.submit(handle_msg, ref(conn), res);
                    break;
                case ServiceCode::query_uname:
                    uid_to_name[stoi(res.uid_)] = res.msg_;
                default:
                    break;
                }

                break;
            }
            else
            {
                continue;
            }
        }
        // else if (n == 0)
        //     {

        //         if (errno == EAGAIN || errno == EWOULDBLOCK)
        //         {
        //             break;
        //         }
        //         else if (errno == EINTR)
        //         {
        //             continue;
        //         }
        //         else
        //         {
        //             conn->excepter_(conn);
        //             exit(0);
        //         }
        //     }
        else
        {
            log_error("server shut down\n");
            break;
        }
    }
    return ret;
}

int signup(unique_ptr<Connection> &conn)
{
    Request r;
    r.servcode_ = ServiceCode::signup;
    int ret;
    cout << format("input your name: ");
    cin >> username;
    r.uid_ = username;
    cout << format("input you passwd: ");
    cin >> r.msg_;
    conn->outbuf_ = r.serialize();
    sender(conn);
    ret = receiver(conn).get();
    return ret;
}
int login(unique_ptr<Connection> &conn)
{
    Request r;
    r.servcode_ = ServiceCode::login;
    int ret;
    cout << format("input your id: ");
    cin >> userid;
    r.uid_ = userid;
    cout << format("input you passwd: ");
    cin >> r.msg_;
    // log_debug("client send: {}", r.serialize());
    conn->outbuf_ = r.serialize();
    sender(conn);
    ret = receiver(conn).get();
    return ret;
}
void query_username(unique_ptr<Connection> &conn, int uid)
{
    // log_debug("query_username trigger\n");
    Request r;
    r.servcode_ = ServiceCode::query_uname,
    r.msg_ = to_string(uid);
    conn->outbuf_ = r.serialize();
    sender(conn);
}
int handle_msg(unique_ptr<Connection> &conn, const Response &res)
{
    int uid = stoi(res.uid_);
    while (!uid_to_name.contains(uid))
    {
        query_username(conn, uid);
        this_thread::sleep_for(100ms);
    }
    cout << format("[{}]# {}\n", uid_to_name[stoi(res.uid_)], res.msg_);
    return 0;
}
int handle_login(const Response &res)
{
    if (res.stuscode_ == StatusCode::error)
    {
        cout << "wrong password or userid\n";
    }
    else if (res.stuscode_ == StatusCode::ok)
    {
        username = res.msg_;
        // log_debug("username: {} userid: {}", username, userid);

        uid_to_name[stoi(userid)] = username;
        cout << format("welcome back, {}\n", username);
    }
    return res.stuscode_ == StatusCode::ok ? 0 : 1;
}
int handle_signup(const Response &res)
{
    if (res.stuscode_ == StatusCode::error)
    {
        cout << "can not sign up for now\n";
    }
    else if (res.stuscode_ == StatusCode::ok)
    {
        cout << format("hello, {},your id is {},please remember.\n", username, res.uid_);
        userid = res.uid_;
    }
    return res.stuscode_ == StatusCode::ok ? 0 : 1;
}

int main()
{
    auto local = make_unique<Sock>(socket(AF_INET, SOCK_STREAM, 0));
    // Sock local(socket(AF_INET,SOCK_STREAM,0));
    local->bd();
    local->conn(serveraddr, port);
    auto conn = make_unique<Connection>(std::move(local), nullptr);

    int choice;
again:
    cout << "input 1 for signup, 2 to login, 0 for exit:> ";
    cin >> choice;
    int notok;
    switch (choice)
    {
    case 1:
        notok = signup(conn);
        break;
    case 2:
        notok = login(conn);
        break;
    default:
        exit(0);
    }
    if (notok)
        goto again;
    cout << "=====CHATNOW=====\n";
    promise<void> pro;
    auto f = pro.get_future();
    workers.submit([&]()->void
                   {
        while (true)
        {
            // //log_debug("receiving msg in child thread");
            receiver(conn);
            if(f.wait_for(1us)!=std::future_status::timeout)break;
        } });
    string msg;
    Request r;
    r.uid_ = userid;
    r.servcode_ = ServiceCode::postmsg;
    cout << "input quit to shut down the program\n";
    while (getline(cin, msg))
    {

        if (!strcasecmp(msg.c_str(), "quit"))
            break;
        r.msg_ = msg;
        conn->outbuf_ = r.serialize();
        sender(conn);
    }
    pro.set_value();
    conn.reset();
    workers.shutdown();
    return 0;
}