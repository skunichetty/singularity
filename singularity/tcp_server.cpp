#include "tcp_server.hpp"

#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>
#include <limits>
#include <stdexcept>
#include <system_error>

#include "utils.hpp"

constexpr static uint32_t MAX_PORT_NUM =
    static_cast<uint32_t>(std::numeric_limits<uint16_t>::max());

TCPServer::TCPServer(uint32_t port) {
    if (port > MAX_PORT_NUM) {
        std::string error_message =
            build_string("Invalid port number ", port,
                         ", expected in range [0,", MAX_PORT_NUM, "]");
        throw std::invalid_argument(error_message);
    } else {
        _port = static_cast<uint16_t>(port);
    }
}

