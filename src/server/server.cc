#include "server/epoll_server.hpp"
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
namespace tinychat{
    tinychat::EpollServer server;
}
int main()
{
    // signal(SIGINT,inthandler);
    signal(SIGCHLD, SIG_IGN);
    // sigignore(SIGCHLD);
    std::string server_address("0.0.0.0:50051");
    auto epc=std::thread([=]{tinychat::RunRpcServer(server_address);});
    tinychat::server.boot();
}
