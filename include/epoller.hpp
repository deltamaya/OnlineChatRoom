#pragma once
#include "minilog.hh"
#include <unistd.h>
#include <fcntl.h>
#include <iostream>
#include <arpa/inet.h>
#include <sys/types.h>
#include <string_view>
#include <cstring>
#include <sys/epoll.h>
#include <cerrno>
#include <vector>
#include "connection.hpp"
#include <format>
namespace tinychat{
    using namespace minilog;
    class Epoller
    {
        static constexpr size_t epoll_num = 64;
        int epollfd_;
    
    public:
        epoll_event events_[epoll_num];
        
        Epoller()
        {
            epollfd_ = ::epoll_create(epoll_num);
            if (epollfd_ < 0)
            {
                ::perror("epoll create");
            }
        }
        ~Epoller(){
            close(epollfd_);
        }
        void modify(int fd, uint32_t event){
            epoll_event ev;
            ev.data.fd=fd;
            ev.events=event;
            int ret=::epoll_ctl(epollfd_,EPOLL_CTL_MOD,fd,&ev);
            if(ret<0){
                log_error("epoll modify failed");
            }
        }
        void add_event(int fd, uint32_t event)
        {
            epoll_event ev;
            ev.data.fd = fd;
            ev.events = event;
            int ret = ::epoll_ctl(epollfd_, EPOLL_CTL_ADD, fd, &ev);
            if (ret < 0)
            {
                ::perror("epoll ctl");
            }
        }
        void delete_event(int fd)
        {
            int ret = ::epoll_ctl(epollfd_, EPOLL_CTL_DEL, fd, nullptr);
            if (ret < 0)
            {
                perror("epoll ctl");
            }
        }
        int wait(int timeout)
        {
            return ::epoll_wait(epollfd_, events_, epoll_num, timeout);
        }
        void rwcfg(std::unique_ptr<Connection>&conn,bool enable_read,bool enable_write){
            this->modify(conn->client_->fd(), (enable_read ? EPOLLIN : 0) | (enable_write ? EPOLLOUT : 0) | EPOLLET);
        }
    };
}