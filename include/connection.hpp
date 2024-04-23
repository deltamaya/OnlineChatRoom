#pragma once

#include "sock.hpp"
#include <sstream>
#include <functional>
#include <ranges>

namespace tinychat{
    class EpollServer;
    
    class Connection{
    public:
        using callback_t=std::function<void(std::unique_ptr<Connection>&)>;
        std::unique_ptr<Sock>client_;
        time_t last_access_;
        std::string inbuf_;
        std::string outbuf_;
        uint32_t event_;
        
        callback_t sender_;
        callback_t receiver_;
        callback_t excepter_;
        EpollServer* svr;
        Connection(std::unique_ptr<Sock> client,EpollServer*p):client_(std::move(client)),last_access_(time(nullptr)),svr(p){
            log_debug("connect {} established",client_->fd());
        }
        ~Connection(){
            log_debug("connect {} closed",client_->fd());
        }
        void register_callback(callback_t receiver,callback_t sender,callback_t excepter){
            sender_=sender;
            receiver_=receiver;
            excepter_=excepter;
        }
    };
}