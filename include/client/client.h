//
// Created by delta on 24/04/2024.
//

#ifndef ONLINE_CHATROOM_CLIENT_H
#define ONLINE_CHATROOM_CLIENT_H

#include "protocol.hpp"
#include "utils.h"
#include <thread>
#include "minilog.hh"
#include <mysql++/mysql++.h>
#include "sock.hpp"
#include "connection.hpp"
#include "thread_pool.hh"
#include "server/epoller.hpp"
#include <regex>
#include "message.pb.h"

namespace tinychat{
    int handle_signup(const Response &res);
    
    int handle_msg(std::unique_ptr<Connection> &conn, const Response &res);
    
    int handle_login(const Response &res);
    
    void handle_query_history(std::unique_ptr<Connection> &conn, const Response &res);
    
    int login(std::unique_ptr<Connection> &conn);
    
    void query_username(std::unique_ptr<Connection> &conn, int uid);
    
    int signup(std::unique_ptr<Connection> &conn);
    
    
    void sender(std::unique_ptr<Connection> &conn);
    
    std::future<int> receiver(std::unique_ptr<Connection> &conn);
    void post_msg(std::unique_ptr<Connection> &conn,std::string);
    
    constexpr uint16_t port = 55369;
    inline std::string userid, pwd, username, group = "null", gid = "null";
    constexpr std::string serveraddr = "127.0.0.1";
    inline ThreadPool<5> workers;
    inline std::unordered_map<int, std::string> uid_to_name;
}
#endif //ONLINE_CHATROOM_CLIENT_H
