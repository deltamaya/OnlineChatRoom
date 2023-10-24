#include "utils.h"
#include <mysql++/mysql++.h>
#include "minilog.hh"
#include <thread>
#include "protocol.hpp"
#include "minilog.hh"
#include "connection.hpp"
#include "epoll_server.hpp"
using namespace minilog;
using namespace std;

void handle_login(unique_ptr<Connection> &conn, const Request &r)
{
    auto dbconn = conn->svr->get();
    Response ret;
    ret.service_ = ServiceCode::login;
    string query_string = format("select * from Users where id={} and pwd=password('{}');", r.uid_, r.msg_);
    log_debug("{}", query_string);
    auto get_user = dbconn->query(query_string);
    get_user.disable_exceptions();
    auto result = get_user.store();
    conn->svr->ret(dbconn);
    if (result.num_rows() > 0)
    {
        ret.uid_ = r.uid_;
        ret.msg_ = string(result[0][1]);
        ret.status_ = StatusCode::ok;
    }
    else
        ret.status_ = StatusCode::error;
    log_debug("handle login ok");

    log_debug("login response: {}", ret.serialize());
    conn->outbuf_ = ret.serialize();
}
void handle_query_username(unique_ptr<Connection> &conn, const Request &r)
{
    auto dbconn = conn->svr->get();
    Response ret;
    ret.service_ = ServiceCode::query_uname;
    string query_string = format("select name from Users where id={};", r.msg_);
    log_debug("{}", query_string);
    auto get_user = dbconn->query(query_string);
    get_user.disable_exceptions();
    auto result = get_user.store();
    conn->svr->ret(dbconn);
    ret.status_ = result ? StatusCode::ok : StatusCode::error;
    if (result.num_rows() > 0)
        ret.msg_ = string(result[0][0]);
    ret.uid_ = r.msg_;
    log_debug("login response: {}", ret.serialize());
    conn->outbuf_ = ret.serialize();
}
void handle_signup(unique_ptr<Connection> &conn, const Request &r)
{
    auto dbconn = conn->svr->get();
    Response ret;

    ret.service_ = ServiceCode::signup;
    string query_string = format("insert into Users(name,pwd) value('{}',password('{}'))", r.uid_, r.msg_);
    log_debug("{}", query_string);
    auto insert = dbconn->query(query_string);
    insert.disable_exceptions();
    auto result = insert.execute();
    conn->svr->ret(dbconn);
    ret.status_ = result ? StatusCode::ok : StatusCode::error;

    string query_auto_increment = format("select last_insert_id();");
    auto auto_increment = dbconn->query(query_auto_increment);
    auto_increment.disable_exceptions();
    auto cur_uid_result = auto_increment.store();
    if (cur_uid_result.num_rows() > 0)
        ret.uid_ = string(cur_uid_result[0][0]);
    conn->outbuf_ = ret.serialize();
}
void handle_postmsg(unique_ptr<Connection> &conn, const Request &r)
{
    auto dbconn = conn->svr->get();
    Response ret;
    string query_string = format("insert into History value({},{},{});", r.uid_, r.to_whom_, r.msg_);
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

        log_debug("sending msg to [{}:{}]", conn->client_->ip(), conn->client_->port());
        conn->svr->epoller_.rwcfg(conn, true, true);
        conn->outbuf_ = ret.serialize();
    }
}