#pragma once
#include <unistd.h>
#include <fcntl.h>
#include <iostream>
#include <arpa/inet.h>
#include <sys/types.h>
#include <string_view>
#include <string.h>
#include <sys/epoll.h>
#include <cerrno>
#include <vector>
#include <format>
#include <string_view>
#include <string>
#include <memory>
#include "minilog.hh"

namespace tinychat{
    using namespace minilog;
    class Sock
    {
        
        static constexpr int backlog = 32;
        const int fd_;
        ::sockaddr_in addr_;
        ::socklen_t len_;
    
    public:
        Sock(int fd, uint16_t port, ::std::string_view ipaddr = "") : fd_(fd), len_(sizeof(addr_))
        {
            int opt = 1;
            ::setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
            if (ipaddr == "")
                addr_.sin_addr.s_addr = INADDR_ANY;
            else
            {
                ::inet_aton(ipaddr.data(), &addr_.sin_addr);
            }
            addr_.sin_family = AF_INET;
            addr_.sin_port = ::htons(port);
        }
        Sock(int fd) : fd_(fd)
        {
            //log_debug("sock {} opened",fd_);
            len_ = sizeof(addr_);
        }
        ~Sock()
        {
            //log_debug("sock {} closed",fd_);
            ::close(fd_);
        }
        [[nodiscard]] int fd()const
        {
            return fd_;
        }
        void listen()
        {
            int ret = ::listen(fd_, backlog);
            if (ret < 0)
            {
                ::perror("listen");
            }
        }
        void bind()
        {
            int ret = ::bind(fd_, (const sockaddr *)(&addr_), len_);
            if (ret < 0)
            {
                ::perror("bind");
            }
        }
        void connect(const std::string&address, const uint16_t port) const{
            sockaddr_in addr{};
            addr.sin_addr.s_addr=inet_addr(address.c_str());
            addr.sin_port=htons(port);
            addr.sin_family=AF_INET;
            auto ret=::connect(fd_,(sockaddr*)(&addr),sizeof(addr));
            if(ret!=0){
                log_error("connect error");
            }
        }
        [[nodiscard]] std::string_view ip()const
        {
            return inet_ntoa(addr_.sin_addr);
        }
        [[nodiscard]] uint16_t port()const
        {
            return ntohs(addr_.sin_port);
        }
        std::unique_ptr<Sock> accept()
        {
            //log_debug("fd:{}",fd_);
            ::sockaddr_in addr{};
            socklen_t len;
            int cfd = ::accept(fd_, (sockaddr *)&addr, &len);
            auto ret=std::make_unique<Sock>(cfd);
            ret->addr_=addr;
            ret->len_=len;
            if (cfd == -1)
            {
                ::perror("accept");
            }
            return ret;
        }
    };
}