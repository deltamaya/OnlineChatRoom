#pragma once
#include "utils.h"
#include <mysql++/mysql++.h>
#include "minilog.hh"
#include <thread>
#include "protocol.hpp"
#include "minilog.hh"
Response handle_login(mysqlpp::Connection*conn,const Request&r);
Response handle_query_username(mysqlpp::Connection *conn, const Request &r);
Response handle_signup(mysqlpp::Connection *conn, const Request &r);