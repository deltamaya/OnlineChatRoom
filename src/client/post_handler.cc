//
// Created by delta on 24/04/2024.
//
#include "client/client.h"

namespace tinychat{
    extern std::unique_ptr<EpollServices::Stub> stub;

    void post_msg(std::unique_ptr<Connection> &conn,std::string msg) {
        if(gid==-1){
            std::cout<<"you need to use 'cd' to focus on a group\n";
            return;
        }
        tinychat::Chat chat;
        chat.set_isgroup(true);
        chat.set_uid(uid);
        chat.set_msg(std::move(msg));
        chat.set_targetid(gid);
        std::string temp=chat.SerializeAsString();
        minilog::log_debug("serialized string: {}",temp);
        conn->outbuf_=temp;
        minilog::log_debug("client out buf: {}",conn->outbuf_);
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
        std::cin >> uid;
        std::string pwd;
        std::cout << std::format("input you passwd: ");
        std::cin >> pwd;
        grpc::ClientContext ctx;
        LoginArg arg;
        LoginReply rep;
        arg.set_uid(uid);
        arg.set_password(pwd);
        auto status=stub->Login(&ctx,arg,&rep);
        return status.ok();
    }
    
    int changeGroup(int newGid){
        grpc::ClientContext ctx;
        ChangeGroupArg arg;
        arg.set_uid(uid);
        arg.set_formergid(gid);
        arg.set_cfd(cfd);
        arg.set_gid(newGid);
        ChangeGroupReply resp;
        stub->ChangeGroup(&ctx,arg,&resp);
        if(resp.ok()){
            gid=newGid;
            std::cout<<"changed to group: "<<newGid<<std::endl;
            
        }else{
            minilog::log_error("change group error");
        }
        return 0;
    }
    
    void query_username(std::unique_ptr<Connection> &conn, int targetId) {
        // log_debug("query_username trigger\n");
//        Request r;
//        r.service_ = ServiceCode::query_uname,
//                r.msg_ = std::to_string(uid);
//        conn->outbuf_ = r.serialize();
//        sender(conn);
        grpc::ClientContext ctx;
        QueryUsernameArg arg;
        arg.set_uid(uid);
        arg.set_targetid(targetId);
        QueryUsernameReply resp;
        stub->QueryUsername(&ctx,arg,&resp);
        uid_to_name[targetId]=resp.username();
    }
}

