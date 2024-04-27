#pragma once
#include "utils.h"
#include "message.pb.h"
#include <mysql++/mysql++.h>
#include "minilog.hh"
#include <thread>
#include "protocol.hpp"
#include "minilog.hh"
#include "connection.hpp"

namespace tinychat{
    void handle_login(std::unique_ptr<tinychat::Connection>&,const Request&r);
    void handle_query_username(std::unique_ptr<tinychat::Connection>&, const Request &r);
    void handle_signup(std::unique_ptr<tinychat::Connection>&, const Request &r);
    void handle_postmsg(std::unique_ptr<tinychat::Connection>&, const Chat &r);
    void handle_query_history(std::unique_ptr<tinychat::Connection>&, const Request &r);
    void handle_cd(std::unique_ptr<tinychat::Connection>&, const Request &r);
    void handle_create_group(std::unique_ptr<tinychat::Connection>&conn,const Request & r);
    void handle_join(std::unique_ptr<tinychat::Connection> &conn, const Request &r);
    void RunRpcServer(std::string);
}