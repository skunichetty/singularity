#pragma once
#ifndef TCP_SERVER_H
#define TCP_SERVER_H

#include <cstdint>
#include <optional>

#include "sockimpl.hpp"

using socket_t = int;

/**
 * @brief The TCPServer class represents a TCP server that listens for incoming
 * connections on a specified port.
 */
class TCPServer {
   private:
    uint16_t _port;  // The port number on which the server listens.
    std::optional<TCPSocket> _socket;

   public:
    /**
     * @brief Constructs a TCPServer object with the specified port number.
     * @param port The port number on which the server listens. Port must be in
     * range [0, 65536]
     */
    TCPServer(uint32_t port);

    /**
     * Starts the TCP server with the specified maximum number of connections.
     *
     * @param max_connections The maximum number of connections allowed by the
     * server.
     */
    void start(int max_connections);
};

#endif  // TCP_SERVER_H
