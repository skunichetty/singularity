#pragma once
#ifndef TCP_SERVER_H
#define TCP_SERVER_H

#include <cstdint>

using socket_t = int;

/**
 * @brief The TCPServer class represents a TCP server that listens for incoming
 * connections on a specified port.
 */
class TCPServer {
   private:
    uint32_t _port;     // The port number on which the server listens.
    socket_t _sock_fd;  // The socket file descriptor used by the server.

   public:
    /**
     * @brief Constructs a TCPServer object with the specified port number.
     * @param port The port number on which the server listens. Port must be in
     * range [0, 65536]
     */
    TCPServer(uint32_t port);

    /**
     * @brief Destroys the TCPServer object.
     */
    ~TCPServer();

    /**
     * @brief Starts the TCP server, making it ready to accept incoming
     * connections.
     */
    void start();

    /**
     * @brief Accepts an incoming connection from a client.
     * @param block Specifies whether the function should block until a
     * connection is accepted.
     * @return The socket file descriptor of the accepted connection.
     */
    socket_t accept(bool block);
};

#endif  // TCP_SERVER_H
