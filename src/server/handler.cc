//
// Created by delta on 23/04/2024.
//
#include "server/epoll_server.hpp"
#include "service.h"
namespace tinychat{
    void handle_login(std::unique_ptr<tinychat::Connection> &conn, const Request &r)
    {
        auto dbconn = conn->svr->get();
        Response ret;
        ret.service_ = ServiceCode::login;
        std::string query_string =std::format("select * from Users where id={} and pwd=password('{}');",r.uid_,r.msg_);
        log_debug("{}", query_string);
        auto get_user = dbconn->query(query_string);

        get_user.disable_exceptions();
        auto result = get_user.store();
        conn->svr->ret(dbconn);
        if (result.num_rows() > 0)
        {
            ret.uid_ = r.uid_;
            ret.msg_ =std::string(result[0][1]);
            ret.status_ = StatusCode::ok;
        }
        else
            ret.status_ = StatusCode::error;
        // log_debug("handle login ok");
        
        log_debug("login response: {}", ret.serialize());
        conn->outbuf_ = ret.serialize();
    }
    void handle_query_username(std::unique_ptr<tinychat::Connection> &conn, const Request &r)
    {
        auto dbconn = conn->svr->get();
        Response ret;
        ret.service_ = ServiceCode::query_uname;
        std::string query_string = format("select name from Users where id={};", r.msg_);
        log_debug("{}", query_string);
        auto get_user = dbconn->query(query_string);
        get_user.disable_exceptions();
        auto result = get_user.store();
        conn->svr->ret(dbconn);
        ret.status_ = result ? StatusCode::ok : StatusCode::error;
        if (result.num_rows() > 0)
            ret.msg_ =std::string(result[0][0]);
        ret.uid_ = r.msg_;
        log_debug("queryname response: {}", ret.serialize());
        conn->outbuf_ = ret.serialize();
    }

    void handle_postmsg(std::unique_ptr<tinychat::Connection> &conn, const tinychat::Chat &r)
    {
        auto dbconn = conn->svr->get();
        std::string query_string = format("insert into History value({},{},'{}');",r.uid(), r.targetid(), r.msg());
        log_debug("{}", query_string);
        auto insert = dbconn->query(query_string);
        insert.disable_exceptions();
        auto result = insert.execute();
        conn->svr->ret(dbconn);
        
        auto &users = conn->svr->gid_to_users_[r.targetid()];
        log_debug("working on gid:{}", r.targetid());
        for (auto &user : users)
        {
            log_debug("sending msg to [{}:{}]", conn->svr->conns_[user]->client_->ip(), conn->svr->conns_[user]->client_->port());
            conn->svr->epoller_.rwcfg(conn->svr->conns_[user], true, true);
            conn->svr->conns_[user]->outbuf_ =r.SerializeAsString();
        }
    }
    
    
}
