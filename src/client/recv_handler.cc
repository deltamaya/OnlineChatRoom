//
// Created by delta on 24/04/2024.
//
#include "client/client.h"

using namespace std::chrono_literals;
namespace tinychat{
    int handle_msg(std::unique_ptr<Connection> &conn, const Chat &res) {
        while (!uid_to_name.contains(res.uid())) {
            query_username(conn, res.uid());
            std::this_thread::sleep_for(100ms);
        }
        std::cout << std::format("[{} : {}]# {}\n", group, uid_to_name[res.uid()], res.msg());
        return 0;
    }
    
    int handle_login(const Response &res) {
        if (res.status_ == StatusCode::error) {
            std::cout << "wrong password or userid\n";
        } else if (res.status_ == StatusCode::ok) {
            username = res.msg_;
            // log_debug("username: {} userid: {}", username, userid);
            
            uid_to_name[stoi(userid)] = username;
            std::cout << std::format("welcome back, {}\n", username);
        }
        return res.status_ == StatusCode::ok ? 0 : 1;
    }
    
    int handle_signup(const Response &res) {
        if (res.status_ == StatusCode::error) {
            std::cout << "can not sign up for now\n";
        } else if (res.status_ == StatusCode::ok) {
            std::cout << std::format("hello, {},your id is {},please remember.\n", username, res.uid_);
            userid = res.uid_;
        }
        return res.status_ == StatusCode::ok ? 0 : 1;
    }
    
    void handle_query_history(std::unique_ptr<Connection> &conn, const Response &res) {
        // log_debug("ok");
        int count = 0, pos = 0, prev = 0;
        int uid = 0;
        if (res.status_ == StatusCode::error)
            return;
        while ((pos = res.msg_.find("#", prev)) != std::string::npos) {
            
            ++count;
            if (count % 2 == 0) {
                std::cout << std::format("[{} : {}]# {}\n", group, uid_to_name[uid], res.msg_.substr(prev, pos - prev));
            } else {
                uid = stoi(res.msg_.substr(prev, pos - prev));
                while (!uid_to_name.contains(uid)) {
                    query_username(conn, uid);
                    std::this_thread::sleep_for(100ms);
                }
            }
            log_debug("prev:{} pos:{}", prev, pos);
            prev = pos + strlen("#");
        }
    }
    
}