#pragma once
#include <array>
#include <functional>
#include <thread>
#include <bitset>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <future>
#include <iostream>
using namespace std;
template <size_t N>
class ThreadPool
{
    vector<promise<void>> shutdown_;
    condition_variable cv_;
    mutex mtx_;
    queue<function<void()>> tasks_;
    vector<thread> workers;

public:
    void shutdown()
    {
        // cout<<"shutdowning\n";
        for (auto &p : shutdown_)
        {
            // cout<<"setting\n";
            p.set_value();
        }
        // cout<<workers.size();
        for (auto &th : workers)
        {
            // cout<<"joinging\n";
            cv_.notify_all();
            if(th.joinable())th.detach();
        }
    }
    ThreadPool() : cv_(), mtx_(), tasks_(), shutdown_(), workers()
    {
        // cout << "initing threadpool\n";
        for (int i = 0; i < N; ++i)
        {
            shutdown_.emplace_back();
        }
        // cout << shutdown_.size();
        // cout << "Im here\n";
        workers.reserve(N);
        for (int i{0}; i < N; ++i)
        {
            // cout << "adding worker\n";
            auto shutdown = shutdown_[i].get_future();
            // cout << "acquireing " << i << " future\n";
            workers.emplace_back([&](future<void>shutdown)
                                 {

            while(shutdown.wait_for(1us)==std::future_status::timeout){
                
                unique_lock<mutex>lk(mtx_);
                while(tasks_.empty()){
                    cv_.wait(lk);
                    if(!(shutdown.wait_for(1us)==std::future_status::timeout)){cv_.notify_all();return;}
                }
                auto task{std::move(tasks_.front())};
                tasks_.pop();
                lk.unlock();
                cv_.notify_one();
                // printf("***************thread %lld: task begin******************\n",this_thread::get_id());
                task();
                // printf("***************thread %lld: task end******************\n",this_thread::get_id());

            } },std::move(shutdown));
        }
    }
    template <typename Func, typename... Args>
    auto submit(Func &&func, Args &&...args) -> future<decltype(func(args...))>
    {
        using rettype = decltype(func(args...));
        auto task = std::make_shared<std::packaged_task<rettype()>>(
            std::bind(std::forward<Func>(func), std::forward<Args>(args)...));
        future<rettype> result{task->get_future()};

        unique_lock<mutex> lk(mtx_);
        tasks_.emplace([task]()
                       { (*task)(); });
        // lg.log(Debug,"task size:{} ",tasks_.size());
        cv_.notify_one();
        return result;
    }
    using iterator = decltype(workers.begin());
    iterator begin()
    {
        return workers.begin();
    }
    iterator end()
    {
        return workers.end();
    }
};