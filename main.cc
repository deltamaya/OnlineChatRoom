#include "epoll_server.hpp"
#include <sys/signal.h>
#include <json/json.h>
int main(){
    signal(SIGCHLD,SIG_IGN);  
    Json::FastWriter w;
    Json::Value v;
    v["user"]="delta";v["code"]=0;v["msg"]="hello arch linux";
    cout<<w.write(v);
    // sigignore(SIGCHLD); 
    EpollServer svr;
    svr.bootup();
}