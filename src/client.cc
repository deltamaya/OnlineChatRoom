#include "protocol.hpp"
#include "utils.h"
#include <thread>
#include "minilog.hh"
#include <mysql++/mysql++.h>
#include "sock.hpp"
#include "connection.hpp"
#include "thread_pool.hh"
#include "epoller.hpp"
#include <regex>
using namespace std;
int handle_signup(const Response &res);
int handle_msg(unique_ptr<Connection> &conn, const Response &res);
int handle_login(const Response &res);
void handle_query_history(unique_ptr<Connection> &conn, const Response &res);
const string serveraddr = "127.0.0.1";
const uint16_t port = 55369;
string userid, pwd, username, group = "null", gid = "null";
ThreadPool<5> workers;
unordered_map<int, string> uid_to_name;
void sender(unique_ptr<Connection> &conn)
{
    while (!conn->outbuf_.empty())
    {
        auto n = send(conn->client_->fd(), conn->outbuf_.c_str(), conn->outbuf_.size(), 0);
        log_debug("sendding n={}", n);
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
                switch (res.service_)
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
                    log_debug("QUERY_NAME ok");
                    uid_to_name[stoi(res.uid_)] = res.msg_;
                    break;
                case ServiceCode::query_history:
                    // log_debug("history");
                    workers.submit(handle_query_history, ref(conn), res);
                    break;
                case ServiceCode::cd:

                    if (res.status_ == StatusCode::error)
                    {
                        cout << "your group id is not correct, use 'show' to show your contacts\n";
                        break;
                    }
                    else
                    {
                        cout << "changed your group now\n";
                        group = res.msg_;
                        gid = res.to_whom_;
                    }

                    break;
                case ServiceCode::create_group:
                    log_debug("creating group");
                    if (res.status_ == StatusCode::ok)
                    {
                        cout << format("your just created the group, group id: {}", res.msg_);
                    }
                    else
                    {
                        cout << format("create group failed");
                    }
                    break;
                case ServiceCode::join:
                    log_debug("join");
                    if (res.status_ == StatusCode::ok)
                    {
                        cout << format("you have successfully joined the group");
                    }
                    else
                    {
                        cout << format("you can't join the group");
                    }
                    break;
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
            workers.shutdown();
            exit(0);
        }
    }
    return ret;
}

int signup(unique_ptr<Connection> &conn)
{
    Request r;
    r.service_ = ServiceCode::signup;
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
    r.service_ = ServiceCode::login;
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
    r.service_ = ServiceCode::query_uname,
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
    cout << format("[{} : {}]# {}\n", group, uid_to_name[stoi(res.uid_)], res.msg_);
    return 0;
}
int handle_login(const Response &res)
{
    if (res.status_ == StatusCode::error)
    {
        cout << "wrong password or userid\n";
    }
    else if (res.status_ == StatusCode::ok)
    {
        username = res.msg_;
        // log_debug("username: {} userid: {}", username, userid);

        uid_to_name[stoi(userid)] = username;
        cout << format("welcome back, {}\n", username);
    }
    return res.status_ == StatusCode::ok ? 0 : 1;
}
int handle_signup(const Response &res)
{
    if (res.status_ == StatusCode::error)
    {
        cout << "can not sign up for now\n";
    }
    else if (res.status_ == StatusCode::ok)
    {
        cout << format("hello, {},your id is {},please remember.\n", username, res.uid_);
        userid = res.uid_;
    }
    return res.status_ == StatusCode::ok ? 0 : 1;
}
void handle_query_history(unique_ptr<Connection> &conn, const Response &res)
{
    // log_debug("ok");
    int count = 0, pos = 0, prev = 0;
    int uid = 0;
    if (res.status_ == StatusCode::error)
        return;
    while ((pos = res.msg_.find("#", prev)) != string::npos)
    {

        ++count;
        if (count % 2 == 0)
        {
            cout << format("[{} : {}]# {}\n", group, uid_to_name[uid], res.msg_.substr(prev, pos - prev));
        }
        else
        {
            uid = stoi(res.msg_.substr(prev, pos - prev));
            while (!uid_to_name.contains(uid))
            {
                query_username(conn, uid);
                this_thread::sleep_for(100ms);
            }
        }
        log_debug("prev:{} pos:{}", prev, pos);
        prev = pos + strlen("#");
    }
}

int main()
{
    auto local = make_unique<Sock>(socket(AF_INET, SOCK_STREAM, 0));
    int opt = 1;
    setsockopt(local->fd(), SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
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
        workers.shutdown();
        exit(0);
    }
    if (notok)
        goto again;
    cout << "=====CHATNOW=====\n";
    promise<void> pro;
    auto f = pro.get_future();
    workers.submit([&]() -> void
                   {
        while (true)
        {
            // //log_debug("receiving msg in child thread");
            receiver(conn);
            if(f.wait_for(1us)!=std::future_status::timeout)break;
        } });
    string msg;
    Request r;
    cout << "input help to get useful info\n";
    while (getline(cin, msg))
    {
        r.uid_ = userid;
        r.service_ = ServiceCode::postmsg;
        r.to_whom_ = gid;
        if (msg.find("#") != string::npos)
        {
            cout << "you cant type '#' because it was used to seperate messages\n";
            continue;
        }
        auto smsg = istringstream(msg);
        string command;
        string body;
        smsg >> command;
        if (!strcasecmp(command.c_str(), "quit"))
        {
            break;
        }
        else if (!strcasecmp(command.c_str(), "show"))
        {
            cout << "uninplemented\n";
            continue;
        }
        else if (!strcasecmp(command.c_str(), "help"))
        {
            cout << format("----------\nsend [your message]\t--to send message to other people\nhistory [line count]\t--to get chat history(less than 100)\nshow\t\t\t--to show your contacts\ncd [group number]\t--change your destination that your message was post\nquit\t\t\t--to quit the program\n----------\n");
        }
        else if (!strcasecmp(command.c_str(), "send"))
        {
            if (gid == "null")
            {
                cout << "you need use 'cd' to focus on a group\n";
                continue;
            }
            r.msg_ = smsg.str().substr(smsg.tellg(), -1);
            conn->outbuf_ = r.serialize();
            sender(conn);
        }
        else if (!strcasecmp(command.c_str(), "creategroup"))
        {
            string group_name;
            smsg >> group_name;
            if (group_name.find(SEP) != string::npos)
            {
                cout << "you can't create a group with '##' because it was used to seperate messages";
                continue;
            }
            r.service_ = ServiceCode::create_group;
            r.msg_ = group_name;
            conn->outbuf_ = r.serialize();
            sender(conn);
        }
        else if (!strcasecmp(command.c_str(), "history"))
        {
            if (gid == "null")
            {
                cout << "you need use 'cd' to focus on a group\n";
                continue;
            }
            smsg >> body;
            int count = 0;
            try
            {
                count = stoi(body);
                if (count > 100)
                {
                    cout << "too large, please type a number less than 100\n";
                    continue;
                }
                r.service_ = ServiceCode::query_history;
                r.msg_ = to_string(count);
                conn->outbuf_ = r.serialize();
                sender(conn);
            }
            catch (...)
            {
                cout << "usage: history [0,100)\n"
                     << endl;
            }
        }
        else if (!strcasecmp(command.c_str(), "join"))
        {
            string groupid;
            smsg >> groupid;
            r.service_ = ServiceCode::join;
            r.msg_ = groupid;
            conn->outbuf_ = r.serialize();
            sender(conn);
        }
        else if (!strcasecmp(command.c_str(), "cd"))
        {
            smsg >> body;
            r.service_ = ServiceCode::cd;
            r.msg_ = body;
            conn->outbuf_ = r.serialize();
            sender(conn);
        }
        else
        {
            cout << "usage error, type 'help' to get more useful info\n";
        }
    }
    pro.set_value();
    conn.reset();
    workers.shutdown();
    return 0;
}
