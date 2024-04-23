#pragma once

#include <cstddef>
#include <cstdint>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <string>
#include <fstream>
#include <filesystem>
#include <sstream>
#include <sys/stat.h>
#include <unordered_map>
#include <cmath>
#include <algorithm>
#include <cstring>
#include <string_view>
#include <iostream>
#include <format>
#include <chrono>

namespace tinychat{
    using namespace std::string_literals;
    using namespace std::string_view_literals;
    
    enum Err : uint8_t
    {
        OK,
        LISTEN_ERR,
        SOCKET_ERR,
        BIND_ERR,
        RECV_ERR,
    };
}

