#include "epoll_server.hpp"
#include <sys/signal.h>
#include <json/json.h>
#include <mysql++/mysql++.h>
#include "utils.h"
#include <mysql++/mysql++.h>
#include "minilog.hh"
#include <thread>
#include "protocol.hpp"
#include "minilog.hh"
#include "service.h"
using namespace minilog;


int main()
{
    // signal(SIGINT,inthandler);
    signal(SIGCHLD, SIG_IGN);
    // sigignore(SIGCHLD);
    tinychat::EpollServer svr;
    svr.boot();
}
