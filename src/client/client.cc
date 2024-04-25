#include "client/client.h"


namespace tinychat{
    using namespace std::chrono_literals;
    
    void sender(std::unique_ptr<Connection> &conn) {
        while (!conn->outbuf_.empty()) {
            auto n = ::send(conn->client_->fd(), conn->outbuf_.c_str(), conn->outbuf_.size(), 0);
            log_debug("sendding n={}", n);
            if (n > 0) {
                conn->outbuf_.erase(0, n);
            } else {
                if (errno == EAGAIN || errno == EWOULDBLOCK)
                    break;
                else if (errno == EINTR)
                    continue;
                else {
                    perror("send");
                    // be careful, excepter will destroy your connection and sock, so it should return instead of break
                    conn->excepter_(conn);
                    return;
                }
            }
        }
    }
    
    std::future<int> receiver(std::unique_ptr<Connection> &conn) {
        std::future<int> ret;
        char buf[bufsize];
        while (1) {
            bzero(buf, sizeof(buf));
            // log_debug("trying to read data from server");
            auto n = recv(conn->client_->fd(), buf, sizeof(buf) - 1, 0);
            if (n > 0) {
                // buf[n-1]=0;buf[n-2]=0;
                conn->inbuf_ += buf;
                // log_debug("now inbuf is : |{}|,length:{}", conn->inbuf_, connect->inbuf_.size());
                Chat resp;
                resp.ParseFromString(conn->inbuf_);
                workers.submit(handle_msg, ref(conn), resp);
                
                
            }
            
            else {
                log_error("server shut down\n");
                workers.shutdown();
                exit(0);
            }
        }
        return ret;
    }
    auto channel=grpc::CreateChannel("localhost:50051",grpc::InsecureChannelCredentials());
    
    std::unique_ptr<EpollServices::Stub> stub=EpollServices::NewStub(channel);
}


using namespace tinychat;
int cfd=-1;


int main() {
    
    auto local = std::make_unique<tinychat::Sock>(socket(AF_INET, SOCK_STREAM, 0));
    int opt = 1;
    setsockopt(local->fd(), SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    // Sock local(socket(AF_INET,SOCK_STREAM,0));
    local->bind();
    local->connect(serveraddr, port);
    auto conn = std::make_unique<Connection>(std::move(local), nullptr);
    char buf[bufsize];
    auto n = recv(conn->client_->fd(), buf, sizeof(buf) - 1, 0);
    cfd=std::stoi(buf);
    minilog::log_debug("my fd is {}",cfd);
    int choice;
again:
    std::cout << "input 1 for signup, 2 to login, 0 for exit:> ";
    std::cin >> choice;
    int notok;
    switch (choice) {
        case 1:
            notok = signup(conn);
            break;
        case 2:
            notok = login(conn);
            break;
        default:
            workers.shutdown();
            exit(0);
    }
    if (notok)
        goto again;
    std::cout << "=====CHATNOW=====\n";
    std::promise<void> pro;
    auto f = pro.get_future();
    workers.submit([&]() -> void {
        while (true) {
            // //log_debug("receiving msg in child thread");
            receiver(conn);
            if (f.wait_for(1us) != std::future_status::timeout)break;
        }
    });
    std::string msg;
    Request r;
    std::cout << "input help to get useful info\n";
    while (getline(std::cin, msg)) {
        r.uid_ = userid;
        r.service_ = ServiceCode::postmsg;
        r.to_whom_ = gid;
        if (msg.find("#") != std::string::npos) {
            std::cout << "you cant type '#' because it was used to seperate messages\n";
            continue;
        }
        auto smsg = std::istringstream(std::move(msg));
        std::string command;
        std::string body;
        smsg >> command;
        smsg >> body;
        if (!strcasecmp(command.c_str(), "quit")) {
            break;
        } else if (!strcasecmp(command.c_str(), "show")) {
            std::cout << "uninplemented\n";
            continue;
        } else if (!strcasecmp(command.c_str(), "help")) {
            std::cout << std::format(
                    "----------\nsend [your message]\t--to send message to other people\nhistory [line count]\t--to get chat history(less than 100)\nshow\t\t\t--to show your contacts\ncd [group number]\t--change your destination that your message was post\nquit\t\t\t--to quit the program\n----------\n");
        } else if (!strcasecmp(command.c_str(), "send")) {
            minilog::log_info("sending msg: {}", body);
            post_msg(conn, std::move(body));
        } else if (!strcasecmp(command.c_str(), "creategroup")) {
            std::string group_name;
            smsg >> group_name;
            if (group_name.find(SEP) != std::string::npos) {
                std::cout << "you can't create a group with '##' because it was used to seperate messages";
                continue;
            }
            r.service_ = ServiceCode::create_group;
            r.msg_ = group_name;
            conn->outbuf_ = r.serialize();
            sender(conn);
        } else if (!strcasecmp(command.c_str(), "history")) {
            if (gid == "null") {
                std::cout << "you need use 'cd' to focus on a group\n";
                continue;
            }
            smsg >> body;
            int count = 0;
            try {
                count = stoi(body);
                if (count > 100) {
                    std::cout << "too large, please type a number less than 100\n";
                    continue;
                }
                r.service_ = ServiceCode::query_history;
                r.msg_ = std::to_string(count);
                conn->outbuf_ = r.serialize();
                sender(conn);
            }
            catch (...) {
                std::cout << "usage: history [0,100)\n"
                          << std::endl;
            }
        } else if (!strcasecmp(command.c_str(), "join")) {
            std::string groupid;
            smsg >> groupid;
            r.service_ = ServiceCode::join;
            r.msg_ = groupid;
            conn->outbuf_ = r.serialize();
            sender(conn);
        } else if (!strcasecmp(command.c_str(), "cd")) {
            smsg >> body;
            r.service_ = ServiceCode::cd;
            r.msg_ = body;
            conn->outbuf_ = r.serialize();
            sender(conn);
        } else {
            std::cout << "usage error, type 'help' to get more useful info\n";
        }
    }
    pro.set_value();
    conn.reset();
    workers.shutdown();
    return 0;
}

