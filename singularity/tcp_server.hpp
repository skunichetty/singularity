#pragma once
#ifndef TCP_SERVER_H
#define TCP_SERVER_H

#include <cstdint>
#include <memory>
#include <optional>
#include <type_traits>

#include "concurrency.hpp"
#include "sockimpl.hpp"
#include "utils.hpp"

namespace singularity::network {

/**
 * @brief The TCPServer class represents a TCP server that listens for incoming
 * connections on a specified port.
 */
class TCPServer {
   private:
    class TCPServerImpl;
    std::unique_ptr<TCPServerImpl> impl;

   public:
    /**
     * @brief Constructs a TCPServer object with the specified port number.
     * @param port The port number on which the server listens. Port must be in
     * range [0, 65536]
     * @throw std::invalid_argument Thrown if port number not in valid
     * range.
     */
    explicit TCPServer(uint32_t port);

    /**
     * Starts the TCP server and listens to connections.
     *
     * Anytime a new connection is intercepted, the connection is added to the
     * connection buffer given.
     *
     * @throw std::system_error Thrown when system is unable to start server.
     * See error message (`what()`) for more information.
     */
    void start(concurrency::Buffer<TCPConnection>& connection_buffer);

    void shutdown();

    ~TCPServer();  // make the type complete
};

}  // namespace singularity::network

#endif  // TCP_SERVER_H
