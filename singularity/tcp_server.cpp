#include "tcp_server.hpp"

#include <unistd.h>

#include <cstring>
#include <limits>
#include <stdexcept>

#include "utils.hpp"

TCPServer::TCPServer(uint32_t port) : _port{port}, _sock_fd{-1} {
    uint32_t max_port =
        static_cast<uint32_t>(std::numeric_limits<uint16_t>::max());
    if ( _port > max_port) {
        throw std::invalid_argument(build_string("Invalid port number ", port,
                                                 ", expected in range [0,",
                                                 max_port, "]"));
    }
}

TCPServer::~TCPServer() { close(_sock_fd); }
