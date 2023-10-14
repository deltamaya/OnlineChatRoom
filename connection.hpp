#pragma once

#include "sock.hpp"
#include <sstream>
#include <functional>
#include <ranges>
using namespace std;
class EpollServer;
class Connection{
    public:
    using callback_t=function<void(unique_ptr<Connection>&)>;
    unique_ptr<Sock>client_;
    time_t last_access_;
    string inbuf_;
    string outbuf_;
    uint32_t event_;

    callback_t sender_;
    callback_t receiver_;
    callback_t excepter_;
    EpollServer* svr;
    Connection(unique_ptr<Sock> client):client_(std::move(client)),last_access_(time(nullptr)),svr(){
        log_debug("conn {} established",client_->fd());
    }
    ~Connection(){
        log_debug("conn {} closed",client_->fd());
    }
    void register_callback(callback_t receiver,callback_t sender,callback_t excepter){
        sender_=sender;
        receiver_=receiver;
        excepter_=excepter;
    }


};