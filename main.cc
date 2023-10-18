#include "epoll_server.hpp"
#include <sys/signal.h>
#include <json/json.h>
#include <mysql++/mysql++.h>
int main(){
    // signal(SIGINT,inthandler);
    signal(SIGCHLD,SIG_IGN);  
    // sigignore(SIGCHLD); 
    EpollServer svr;
    svr.bootup();
}
