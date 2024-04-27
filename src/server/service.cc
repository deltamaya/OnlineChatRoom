#include "client/client.h"
#include "server/epoll_server.hpp"
namespace tinychat{
    extern EpollServer server;
    class EpollServicesImpl final:public EpollServices::Service{
        ::grpc::Status Login(::grpc::ServerContext* context, const ::tinychat::LoginArg* request,
                             ::tinychat::LoginReply* response) override{
            auto dbconn=server.get();
            std::string query_string =std::format("select * from Users where id={} and pwd=password('{}');",request->uid(),
                                                  request->password());
            log_debug("{}", query_string);
            auto get_user = dbconn->query(query_string);
            get_user.disable_exceptions();
            auto result = get_user.store();
            server.ret(dbconn);
            if (result.num_rows() > 0)
            {
                response->set_ok(true);
                return grpc::Status::OK;
            }
            return grpc::Status::CANCELLED;
        }
        
        ::grpc::Status SignUp(::grpc::ServerContext* context, const ::tinychat::SignUpArg* request, ::tinychat::SignUpReply* response) override{
            auto dbconn=server.get();
            std::string query_string = format("insert into Users(name,pwd) value('{}',password('{}'))", request->username(),
                                              request->password());
            auto insert=dbconn->query(query_string);
            insert.disable_exceptions();
            auto result=insert.execute();
            server.ret(dbconn);
            if(result){
                response->set_uid(result.insert_id());
                return ::grpc::Status(::grpc::StatusCode::OK, "");
            }
            
            
            return ::grpc::Status(::grpc::StatusCode::NOT_FOUND, "");
        }
        
        ::grpc::Status CreateGroup(::grpc::ServerContext* context, const ::tinychat::CreateGroupArg* request, ::tinychat::CreateGroupReply* response)override {
            auto dbconn = server.get();
            if(!request->has_uid()){
                return ::grpc::Status(::grpc::StatusCode::UNAVAILABLE, "");;
            }
            std::string query_string = format("insert into Groups(name) value('{}');",request->name());
            log_debug("{}", query_string);
            auto query = dbconn->query(query_string);
            query.disable_exceptions();
            auto result = query.execute();
            auto groupid = result.insert_id();
            server.ret(dbconn);
            
            if (result)
            {
                response->set_gid(groupid);
                server.gid_to_users_.emplace(groupid, std::unordered_set<int>{});
                return ::grpc::Status(::grpc::StatusCode::OK, "");
                
            }
            return ::grpc::Status(::grpc::StatusCode::NOT_FOUND, "");
        }
        
        ::grpc::Status QueryUsername(::grpc::ServerContext* context, const ::tinychat::QueryUsernameArg* request,
                                     ::tinychat::QueryUsernameReply* response) override{
            auto dbconn = server.get();
            if(!request->has_uid()){
                return ::grpc::Status(::grpc::StatusCode::CANCELLED, "");;
            }
            std::string query_string = std::format("select name from Users where id={};", request->targetid());
            log_debug("{}", query_string);
            auto get_user = dbconn->query(query_string);
            get_user.disable_exceptions();
            auto result = get_user.store();
            server.ret(dbconn);
            if(result.num_rows()>0){
                response->set_username(std::string(result[0][0]));
                return ::grpc::Status(::grpc::StatusCode::OK, "");
                
            }
            return ::grpc::Status(::grpc::StatusCode::NOT_FOUND, "");
        }
        
        ::grpc::Status ChangeGroup(::grpc::ServerContext* context, const ::tinychat::ChangeGroupArg* request, ::tinychat::ChangeGroupReply* response) override{
            auto dbconn =server.get();
            std::string query_string = std::format("select name from Groups where id={};", request->gid());
            log_debug("{}", query_string);
            auto insert = dbconn->query(query_string);
            insert.disable_exceptions();
            auto result = insert.store();
            if (result.num_rows() > 0)
            {
                std::string exist_query_string = std::format("select * from UserGroup where uid={} and gid={};", request->uid(),
                                                             request->gid());
                log_debug("{}", exist_query_string);
                auto exists = dbconn->query(query_string);
                exists.disable_exceptions();
                auto existsresult = exists.store();
                if (existsresult.num_rows() > 0)
                {
                    log_debug("num row is {}",existsresult.num_rows());
                    response->set_ok(true);
                    if(request->has_formergid()){
                        server.gid_to_users_[request->formergid()].erase(request->cfd());
                    }
                    server.gid_to_users_[request->gid()].insert(request->cfd());
                }
                else
                {
                    log_debug("no user in the group");
                }
            }
            response->set_ok(true);
            server.ret(dbconn);
            return ::grpc::Status(::grpc::StatusCode::OK, "");
        }
        
        ::grpc::Status JoinGroup(::grpc::ServerContext* context, const ::tinychat::JoinGroupArg* request, ::tinychat::JoinGroupReply* response) override{
            (void) context;
            (void) request;
            (void) response;
            return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
        }
        
        ::grpc::Status QueryHistory(::grpc::ServerContext* context, const ::tinychat::QueryHistoryArg* request, ::tinychat::QueryHistoryReply* response)override {
            (void) context;
            (void) request;
            (void) response;
            return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
        }
    };
    void RunRpcServer(std::string server_address) {
        EpollServicesImpl service;
        grpc::ServerBuilder builder;
        builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
        builder.RegisterService(&service);
        std::unique_ptr<grpc::Server> rpcServer(builder.BuildAndStart());
        minilog::log_info("Server listening on {}" , server_address);
        
        rpcServer->Wait();

    }
}