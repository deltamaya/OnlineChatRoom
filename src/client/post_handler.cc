//
// Created by delta on 24/04/2024.
//
#include "client/client.h"

namespace tinychat{
    void post_msg(std::unique_ptr<Connection> &conn,std::string msg) {
        if(gid=="null"){
            std::cout<<"you need to use 'cd' to focus on a group\n";
            return;
        }
        tinychat::Chat chat;
        chat.set_isgroup(true);
        chat.set_uid(std::stoi(userid));
        chat.set_msg(std::move(msg));
        chat.set_targetid(std::stoi(gid));
        std::string temp;
        chat.SerializeToString(&temp);
        minilog::log_debug("serialized string: {}",temp);
        conn->outbuf_=temp;
        sender(conn);
    }
    
    int signup(std::unique_ptr<Connection> &conn) {
        Request r;
        r.service_ = ServiceCode::signup;
        int ret;
        std::cout << std::format("input your name: ");
        std::cin >> username;
        r.uid_ = username;
        std::cout << std::format("input you passwd: ");
        std::cin >> r.msg_;
        conn->outbuf_ = r.serialize();
        sender(conn);
        ret = receiver(conn).get();
        return ret;
    }
    
    int login(std::unique_ptr<Connection> &conn) {
        Request r;
        r.service_ = ServiceCode::login;
        int ret;
        std::cout << std::format("input your id: ");
        std::cin >> userid;
        r.uid_ = userid;
        std::cout << std::format("input you passwd: ");
        std::cin >> r.msg_;
        // log_debug("client send: {}", r.serialize());
        conn->outbuf_ = r.serialize();
        sender(conn);
        ret = receiver(conn).get();
        return ret;
    }
    
    void query_username(std::unique_ptr<Connection> &conn, int uid) {
        // log_debug("query_username trigger\n");
        Request r;
        r.service_ = ServiceCode::query_uname,
                r.msg_ = std::to_string(uid);
        conn->outbuf_ = r.serialize();
        sender(conn);
    }
}

