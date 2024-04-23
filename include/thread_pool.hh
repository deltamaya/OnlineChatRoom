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
namespace tinychat{
    template <size_t N>
    class ThreadPool
    {
        bool shutdown_;
        std::condition_variable cv_;
        std::mutex mtx_;
        std::queue<std::function<void()>> tasks_;
        std::vector<std::thread> workers;
    
    public:
        void shutdown()
        {
            shutdown_=true;
            for (auto &th : workers)
            {
                cv_.notify_all();
                if(th.joinable())th.detach();
            }
        }
        ThreadPool() : cv_(), mtx_(), tasks_(), shutdown_(), workers()
        {
            
            shutdown_=false;
            workers.reserve(N);
            for (int i{0}; i < N; ++i)
            {
                workers.emplace_back([&]()
                                     {
                                         
                                         while(!shutdown_){
                                             
                                             std::unique_lock<std::mutex>lk(mtx_);
                                             while(tasks_.empty()){
                                                 cv_.wait(lk);
                                                 if(shutdown_)return;
                                             }
                                             auto task{std::move(tasks_.front())};
                                             tasks_.pop();
                                             lk.unlock();
                                             cv_.notify_one();
                                             task();
                                             
                                         }} );
            }
        }
        template <typename Func, typename... Args>
        auto submit(Func &&func, Args &&...args) -> std::future<decltype(func(args...))>
        {
            using rettype = decltype(func(args...));
            auto task = std::make_shared<std::packaged_task<rettype()>>(
                    std::bind(std::forward<Func>(func), std::forward<Args>(args)...));
            std::future<rettype> result{task->get_future()};
            
            std::unique_lock<std::mutex> lk(mtx_);
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
}