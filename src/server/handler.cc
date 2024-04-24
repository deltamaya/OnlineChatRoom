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
    void handle_signup(std::unique_ptr<tinychat::Connection> &conn, const Request &r)
    {
        auto dbconn = conn->svr->get();
        Response ret;
        
        ret.service_ = ServiceCode::signup;
        std::string query_string = format("insert into Users(name,pwd) value('{}',password('{}'))", r.uid_, r.msg_);
        log_debug("{}", query_string);
        auto insert = dbconn->query(query_string);
        insert.disable_exceptions();
        auto result = insert.execute();
        conn->svr->ret(dbconn);
        ret.status_ = result ? StatusCode::ok : StatusCode::error;
        
        //std::string query_auto_increment = format("select last_insert_id();");
        // auto auto_increment = dbconn->query(query_auto_increment);
        // auto_increment.disable_exceptions();
        // auto cur_uid_result = auto_increment.store();
        // if (cur_uid_result.num_rows() > 0)
        // ret.uid_ =std::string(cur_uid_result[0][0]);
        ret.uid_ = std::to_string(result.insert_id());
        conn->outbuf_ = ret.serialize();
    }
    void handle_postmsg(std::unique_ptr<tinychat::Connection> &conn, const Request &r)
    {
        auto dbconn = conn->svr->get();
        Response ret;
        std::string query_string = format("insert into History value({},{},'{}');", r.uid_, r.to_whom_, r.msg_);
        log_debug("{}", query_string);
        auto insert = dbconn->query(query_string);
        insert.disable_exceptions();
        auto result = insert.execute();
        conn->svr->ret(dbconn);
        ret.status_ = result ? StatusCode::ok : StatusCode::error;
        ret.uid_ = r.uid_;
        ret.msg_ = r.msg_;
        ret.to_whom_ = r.to_whom_;
        ret.service_ = ServiceCode::postmsg;
        auto &users = conn->svr->gid_to_users_[stoi(r.to_whom_)];
        log_debug("working on gid:{}", stoi(r.to_whom_));
        for (auto &user : users)
        {
            log_debug("sending msg to [{}:{}]", conn->svr->conns_[user]->client_->ip(), conn->svr->conns_[user]->client_->port());
            conn->svr->epoller_.rwcfg(conn->svr->conns_[user], true, true);
            conn->svr->conns_[user]->outbuf_ = ret.serialize();
        }
    }
    void handle_cd(std::unique_ptr<tinychat::Connection> &conn, const Request &r)
    {
        auto dbconn = conn->svr->get();
        Response ret;
        std::string query_string = format("select name from Groups where id={};", r.msg_);
        log_debug("{}", query_string);
        auto insert = dbconn->query(query_string);
        insert.disable_exceptions();
        auto result = insert.store();
        ret.service_=ServiceCode::cd;
        if (result.num_rows() > 0)
        {
            std::string exist_query_string = format("select * from UserGroup where uid={} and gid={};", r.uid_, r.msg_);
            log_debug("{}", exist_query_string);
            auto exists = dbconn->query(query_string);
            exists.disable_exceptions();
            auto existsresult = exists.store();
            if (existsresult.num_rows() > 0)
            {
                log_debug("num row is {}",existsresult.num_rows());
                ret.status_ = StatusCode::ok;
                ret.uid_ = r.uid_;
                ret.msg_ =std::string(result[0][0]);
                ret.to_whom_ = r.msg_;
                if (r.to_whom_ != "null")
                {
                    conn->svr->gid_to_users_[stoi(r.msg_)].erase(conn->client_->fd());
                }
                conn->svr->gid_to_users_[stoi(r.msg_)].insert(conn->client_->fd());
            }
            else
            {
                log_debug("no user in the group");
                ret.status_ = StatusCode::error;
            }
        }
        else
        {
            ret.status_ = StatusCode::error;
        }
        log_debug("{}", ret.serialize());
        conn->svr->ret(dbconn);
        conn->outbuf_ = ret.serialize();
    }
    void handle_query_history(std::unique_ptr<tinychat::Connection> &conn, const Request &r)
    {
        auto dbconn = conn->svr->get();
        Response ret;
        std::string query_string = format("select uid,msg from History where gid={} limit {};", r.to_whom_, r.msg_);
        log_debug("{}", query_string);
        auto query = dbconn->query(query_string);
        query.disable_exceptions();
        auto result = query.store();
        result.disable_exceptions();
        ret.service_ = ServiceCode::query_history;
        ret.uid_ = r.uid_;
        ret.to_whom_ = r.to_whom_;
        if (result.num_rows() > 0)
        {
            ret.status_ = StatusCode::ok;
            for (int i = 0; i < result.num_rows(); i++)
            {
                ret.msg_ += format("{}#{}#",std::string(result[i][0]),std::string(result[i][1]));
                log_debug("{}", format("{}#{}#",std::string(result[i][0]),std::string(result[i][1])));
            }
        }
        else
        {
            ret.status_ = StatusCode::error;
        }
        log_debug("{}", ret.serialize());
        conn->svr->ret(dbconn);
        conn->outbuf_ = ret.serialize();
    }
    void handle_create_group(std::unique_ptr<tinychat::Connection> &conn, const Request &r)
    {
        auto dbconn = conn->svr->get();
        Response ret;
        ret.service_=ServiceCode::create_group;
        std::string query_string = format("insert into Groups(name) value('{}');", r.msg_);
        log_debug("{}", query_string);
        auto query = dbconn->query(query_string);
        query.disable_exceptions();
        auto result = query.execute();
        auto groupid = result.insert_id();
        if (result)
        {
            ret.status_ = StatusCode::ok;
            ret.msg_ = groupid;
            conn->svr->gid_to_users_.emplace(groupid, std::unordered_set<int>{});
        }
        else
        {
            ret.status_ = StatusCode::error;
        }
        conn->svr->ret(dbconn);
        conn->outbuf_ = ret.serialize();
    }
    void handle_join(std::unique_ptr<tinychat::Connection> &conn, const Request &r)
    {
        auto dbconn = conn->svr->get();
        Response ret;
        ret.service_=ServiceCode::join;
        std::string query_string = format("insert into UserGroup value({},{});", r.uid_, r.msg_);
        log_debug("{}", query_string);
        auto query = dbconn->query(query_string);
        query.disable_exceptions();
        auto result = query.execute();
        if (result)
        {
            ret.status_ = StatusCode::ok;
        }
        else
        {
            ret.status_ = StatusCode::error;
        }
        conn->svr->ret(dbconn);
        conn->outbuf_ = ret.serialize();
    }
}
