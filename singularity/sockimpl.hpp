#pragma once
#ifndef SOCKET_IMPL_H
#define SOCKET_IMPL_H

#include <netinet/in.h>
#include <sys/types.h>

#include <cstdint>
#include <string>

using socket_t = int;

/**
 * @brief Represents a socket address.
 *
 * The `SocketAddress` class provides a representation of a socket address.
 * It encapsulates the underlying `sockaddr_storage` structure and provides
 * methods to access the address family, length, and data of the socket address.
 */
class SocketAddress {
   protected:
    sockaddr_storage _address;
    SocketAddress() = default;

   public:
    /**
     * @brief Returns the address family of the socket address.
     * @return The address family of the socket address.
     */
    [[nodiscard]] sa_family_t sa_family() const;

    /**
     * @brief Returns the length of the socket address.
     * @return The length of the socket address.
     */
    [[nodiscard]] socklen_t length() const;

    /**
     * @brief Returns a pointer to the underlying `sockaddr` structure.
     * @return A pointer to the underlying `sockaddr` structure.
     */
    [[nodiscard]] const sockaddr* data() const;
};

/**
 * @brief Represents an IP socket address.
 *
 * The `IPSocketAddress` class is a subclass of `SocketAddress` and represents
 * an IP socket address. It provides methods to retrieve the IP address and port
 * associated with the socket address.
 */
class IPSocketAddress : public SocketAddress {
   private:
    sockaddr_in* _ip_addr;

   public:
    /**
     * @brief Constructs an `IPSocketAddress` object with the specified IP
     * address and port.
     *
     * @param address The IP address in network byte order.
     * @param port The port number.
     */
    IPSocketAddress(uint32_t address, uint16_t port);

    /**
     * @brief Constructs an `IPSocketAddress` object with the specified IP
     * address and port.
     *
     * @param address The IP address as a string.
     * @param port The port number.
     */
    IPSocketAddress(const char* address, uint16_t port);

    /**
     * @brief Constructs an `IPSocketAddress` object with the specified socket
     * address.
     *
     * @param address The socket address.
     * @param addrlen The length of the socket address.
     */
    IPSocketAddress(const sockaddr* address, socklen_t addrlen);

    /**
     * @brief Copy constructor.
     *
     * @param other The `IPSocketAddress` object to copy from.
     */
    IPSocketAddress(const IPSocketAddress& other);

    /**
     * @brief Copy assignment operator.
     *
     * @param other The `IPSocketAddress` object to assign from.
     * @return A reference to the assigned `IPSocketAddress` object.
     */
    IPSocketAddress& operator=(const IPSocketAddress& other);

    /**
     * @brief Move constructor.
     *
     * @param other The `IPSocketAddress` object to move from.
     */
    IPSocketAddress(IPSocketAddress&& other) = default;

    /**
     * @brief Move assignment operator.
     *
     * @param other The `IPSocketAddress` object to assign from.
     * @return A reference to the assigned `IPSocketAddress` object.
     */
    IPSocketAddress& operator=(IPSocketAddress&& other) = default;

    /**
     * @brief Returns the IP address in network byte order.
     *
     * @return The IP address in network byte order.
     */
    [[nodiscard]] uint32_t address() const;

    /**
     * @brief Returns the IP address as a string.
     *
     * @return The IP address as a string.
     */
    [[nodiscard]] std::string address_str() const;

    /**
     * @brief Returns the port number.
     *
     * @return The port number.
     */
    [[nodiscard]] uint16_t port() const;
};

/**
 * @brief Represents a TCP socket.
 *
 * This class provides functionality to create, bind, listen, accept
 * connections, and disconnect TCP sockets.
 */
class TCPSocket {
   private:
    socket_t _socket;

   public:
    /**
     * @brief Default constructor for TCPSocket.
     */
    TCPSocket();

    /**
     * @brief Binds the socket to the specified IP socket address.
     *
     * @param info The IP socket address to bind to.
     */
    void bind_address(const IPSocketAddress& info);

    /**
     * @brief Starts listening for incoming connections on the socket.
     *
     * @param max_connections The maximum number of connections that can be
     * queued for acceptance.
     */
    void start_listen(int max_connections);

    /**
     * @brief Accepts an incoming connection and returns the socket and the IP
     * socket address of the remote endpoint.
     *
     * @return A pair containing the accepted socket and the IP socket address
     * of the remote endpoint.
     */
    std::pair<socket_t, IPSocketAddress> accept_ctx();

    /**
     * @brief Disconnects the socket.
     */
    void disconnect();

    /**
     * @brief Destructor for TCPSocket.
     */
    ~TCPSocket();
};

#endif  // SOCKET_IMPL_H