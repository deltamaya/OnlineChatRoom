//
// Created by delta on 24/04/2024.
//
#include "client/client.h"
namespace tinychat{
    extern std::unique_ptr<EpollServices::Stub> stub;
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
        std::cout << std::format("input your name: ");
        std::cin >> username;
        std::string pwd;
        std::cout << std::format("input you passwd: ");
        std::cin >> pwd;
        grpc::ClientContext ctx;
        SignUpArg arg;
        SignUpReply rep;
        arg.set_username(username);
        arg.set_password(pwd);
        auto status=stub->SignUp(&ctx,arg,&rep);
        return status.ok();
    }
    
    int login(std::unique_ptr<Connection> &conn) {

        std::cout << std::format("input your id: ");
        std::cin >> userid;
        std::string pwd;
        std::cout << std::format("input you passwd: ");
        std::cin >> pwd;
        grpc::ClientContext ctx;
        LoginArg arg;
        LoginReply rep;
        arg.set_uid(std::stoi(userid));
        arg.set_password(pwd);
        auto status=stub->Login(&ctx,arg,&rep);
        return status.ok();
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

