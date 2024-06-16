#pragma once
#ifndef TCP_SERVER_H
#define TCP_SERVER_H

#include <cstdint>
#include <memory>
#include <optional>
#include <type_traits>

#include "sockimpl.hpp"
#include "utils.hpp"

/**
 * @brief The TCPServer class represents a TCP server that listens for incoming
 * connections on a specified port.
 */
class TCPServer {
   public:
    class TCPServerImpl;
    std::unique_ptr<TCPServerImpl> impl = nullptr;

    /**
     * @brief Constructs a TCPServer object with the specified port number.
     * @param port The port number on which the server listens. Port must be in
     * range [0, 65536]
     * @throw std::invalid_argument Thrown if port number not in valid
     * range.
     */
    explicit TCPServer(uint32_t port);

    /**
     * Starts the TCP server with the specified maximum number of connections.
     *
     * @param max_connections The maximum number of connections allowed by the
     * server.
     * @throw std::system_error Thrown when system is unable to start server.
     * See error message (`what()`) for more information.
     */
    void start(int max_queued_connections);

    template <typename DurationType>
        requires IsDuration<DurationType>
    TCPConnection accept_connection(
        std::optional<DurationType> timeout = std::nullopt);

    void shutdown();
};

#endif  // TCP_SERVER_H
