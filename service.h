#pragma once
#include "utils.h"
#include <mysql++/mysql++.h>
#include "minilog.hh"
#include <thread>
#include "protocol.hpp"
#include "minilog.hh"
void handle_login(unique_ptr<Connection>&,const Request&r);
void handle_query_username(unique_ptr<Connection>&, const Request &r);
void handle_signup(unique_ptr<Connection>&, const Request &r);
void handle_postmsg(unique_ptr<Connection>&, const Request &r);
void handle_query_history(unique_ptr<Connection>&, const Request &r);
void handle_cd(unique_ptr<Connection>&, const Request &r);