#include "utils.h"
#include <mysql++/mysql++.h>
#include "minilog.hh"
#include <thread>
#include "protocol.hpp"
#include "minilog.hh"
using namespace minilog;
using namespace std;

Response handle_login(mysqlpp::Connection *conn, const Request &r)
{
    Response ret;
    ret.servcode_ = ServiceCode::login;
    string query_string = format("select * from Users where id={} and pwd=password('{}');", r.uid_, r.msg_);
    log_debug("{}", query_string);
    auto get_user = conn->query(query_string);
    get_user.disable_exceptions();
    auto result = get_user.store();
    ret.stuscode_ = result ? StatusCode::ok : StatusCode::error;
    if (result)
        ret.msg_ = string(result[0][1]);
    return ret;
}
Response handle_query_username(mysqlpp::Connection *conn, const Request &r)
{
    Response ret;
    ret.servcode_ = ServiceCode::query_uname;
    string query_string = format("select name from Users where id={};", r.msg_);
    log_debug("{}", query_string);
    auto get_user = conn->query(query_string);
    get_user.disable_exceptions();
    auto result = get_user.store();
    ret.stuscode_ = result ? StatusCode::ok : StatusCode::error;
    if (result)
        ret.msg_ = string(result[0][0]);
    ret.uid_=r.msg_;
    return ret;
}

// int signup()
// {
// }
// int login()
// {
//     mysqlpp::StoreQueryResult result;
//     do
//     {
//         cout << format("input your id: ");
//         cin >> userid;
//         cout << format("input you passwd: ");
//         cin >> pwd;
//         string query_string=format("select * from Users where id={} and pwd=password('{}');",userid,pwd);
//         auto get_user = conn.query(query_string);
//         log_debug("{}",query_string);
//         get_user.disable_exceptions();
//         result=get_user.store();
//         if(!result)cout<<"wrong password\n";
//     } while (result.empty());
//     string uid(result[0][0]);
//     string uname(result[0][1]);
//     cout<<format("hello, {}, your id is {}\n",uname,uid);
//     return 0;
// }