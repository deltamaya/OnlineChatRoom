#pragma once
#include <array>
#include <thread>
#include <queue>
#include <mutex>
#include <vector>
#include <future>
#include <mysql++/mysql++.h>
#include "utils.h"
#include "minilog.hh"
namespace tinychat{
    using namespace minilog;
    using namespace std::chrono_literals;
    constexpr const char *host = "localhost";
    constexpr const  char *user = "root";
    constexpr const char *passwd = "";
    constexpr const char *db = "ChatRoom";
    template <size_t N>
    class DBConnPool
    {
        std::condition_variable cv_;
        std::mutex mtx_;
        std::queue<mysqlpp::Connection*>ready_;
        std::vector<mysqlpp::Connection> conns_;
    
    public:
        DBConnPool()
        {
            conns_.resize(N);
            for (int i=0;i<N;++i)
            {
                auto conn=&conns_[i];
                conn->disable_exceptions();
                int retry=0;
                while (!conn->connect(db, host, user, passwd, 8080))
                {
                    if (retry > 10)
                    {
                        log_error("connection fail, quit\n") ;
                        exit(1);
                    }
                    std::cout << "trying to connect to database\n";
                    std::this_thread::sleep_for(1s);
                    ++retry;
                }
                std::cout << "connected\n";
                ready_.push(conn);
            }
        }
        mysqlpp::Connection* get(){
            std::unique_lock<std::mutex>lk(mtx_);
            while(ready_.empty())cv_.wait(lk);
            auto ret=ready_.front();
            ready_.pop();
            lk.unlock();
            return ret;
        }
        void ret(mysqlpp::Connection*conn){
            std::unique_lock<std::mutex>lk(mtx_);
            ready_.push(conn);
            lk.unlock();
            cv_.notify_one();
        }
    };
}