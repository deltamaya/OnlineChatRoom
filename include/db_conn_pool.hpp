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
using namespace minilog;
using namespace std;
const char *host = "localhost";
const char *user = "root";
const char *passwd = "";
const char *db = "ChatRoom";
template <size_t N>
class DBConnPool
{
    condition_variable cv_;
    mutex mtx_;
    queue<mysqlpp::Connection*>ready_;
    vector<mysqlpp::Connection> conns_;

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
                cout << "trying to connect to database\n";
                this_thread::sleep_for(1s);
                ++retry;
            }
            cout << "connected\n";
            ready_.push(conn);
        }
    }
    mysqlpp::Connection* get(){
        unique_lock<mutex>lk(mtx_);
        while(ready_.empty())cv_.wait(lk);
        auto ret=ready_.front();
        ready_.pop();
        lk.unlock();
        return ret;
    }
    void ret(mysqlpp::Connection*conn){
        unique_lock<mutex>lk(mtx_);
        ready_.push(conn);
        lk.unlock();
        cv_.notify_one();
    }
};