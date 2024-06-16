#pragma once
#ifndef SOCKET_IMPL_H
#define SOCKET_IMPL_H

#include <netinet/in.h>
#include <sys/types.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>

using socket_t = int;

/**
 * @brief Represents a socket address.
 *
 * The `SocketAddress` class provides a base class for representing socket
 * addresses. It contains methods for retrieving information about the socket
 * address.
 */
class SocketAddress {
   protected:
    sockaddr_storage _address;
    SocketAddress() = default;

   public:
    [[nodiscard]] sa_family_t sa_family() const;
    [[nodiscard]] virtual socklen_t length() const = 0;
    [[nodiscard]] const sockaddr* data() const;
};

/**
 * @brief Represents an IP socket address.
 *
 * The `IPSocketAddress` class is a subclass of `SocketAddress` and represents
 * an IP socket address. It provides methods to retrieve the IP address, port
 * number, and length of the address.
 *
 * This class supports both IPv4 and IPv6 addresses.
 */
class IPSocketAddress : public SocketAddress {
   private:
    sockaddr_in* _ip_addr;

   public:
    IPSocketAddress(uint32_t address, uint16_t port);
    IPSocketAddress(const char* address, uint16_t port);
    IPSocketAddress(const sockaddr* address, socklen_t addrlen);

    IPSocketAddress(const IPSocketAddress& other);
    IPSocketAddress(IPSocketAddress&& other) = default;

    IPSocketAddress& operator=(const IPSocketAddress& other);
    IPSocketAddress& operator=(IPSocketAddress&& other) = default;

    [[nodiscard]] uint32_t address() const;
    [[nodiscard]] std::string address_str() const;
    [[nodiscard]] uint16_t port() const;
    [[nodiscard]] socklen_t length() const override;
};

/**
 * @brief Represents a message buffer that holds raw data.
 */
class MessageBuffer {
   private:
    std::unique_ptr<std::byte> _data;
    size_t _length;

   public:
    MessageBuffer(const void* data, size_t datasize);

    /**
     * @brief Creates a MessageBuffer object from a string.
     * @param message The string to create the buffer from.
     * @return The created MessageBuffer object.
     */
    static MessageBuffer from_string(const std::string& message);

    /**
     * @brief Creates a MessageBuffer object from a string view.
     * @param view The string view to create the buffer from.
     * @return The created MessageBuffer object.
     */
    static MessageBuffer from_string(std::string_view view);

    /**
     * @brief Creates a MessageBuffer object from a C-style string.
     * @param message The C-style string to create the buffer from.
     * @return The created MessageBuffer object.
     */
    static MessageBuffer from_string(const char* message);

    [[nodiscard]] const std::byte* raw() const;
    [[nodiscard]] size_t length() const;
};

/**
 * @brief Represents a TCP connection.
 *
 * The TCPConnection class provides functionality to establish and manage a TCP
 * connection. It allows sending and receiving messages over the connection.
 */
class TCPConnection {
   private:
    std::optional<socket_t> _socket;
    IPSocketAddress _address;

   public:
    /**
     * @brief Constructs a TCPConnection object with an existing socket and
     * client address.
     *
     * Intended for server-side usage - for example, creating a
     * managed connection from a client socket returned from `accept()`.
     *
     * @param sock_fd The file descriptor of the socket.
     * @param client_address The IP socket address of the client.
     */
    TCPConnection(socket_t sock_fd, const IPSocketAddress& client_address);

    /**
     * @brief Constructs a TCPConnection object with a server address.
     *
     * Intended for client-side usage - for example, creating an instance of
     * `TCPConnection` and then calling `TCPConnection::open()` to connect to
     * the server.
     *
     * @param server_address The IP socket address of the server.
     */
    explicit TCPConnection(const IPSocketAddress& server_address);

    /**
     * @brief Sends a message over the TCP connection.
     *
     * @param buffer The message buffer containing the data to be sent.
     *
     * @throw std::system_error Operating system was unable to send data
     * successfully .
     * @throw InactiveConnectionError Connection was inactive.
     */
    void send_message(const MessageBuffer& buffer);

    /**
     * @brief Receives a message from the TCP connection.
     *
     * @return The received message buffer.
     *
     * @throw std::system_error Operating system was unable to receive data
     * successfully.
     * @throw InactiveConnectionError Connection was inactive.
     */
    MessageBuffer receive_message();

    /**
     * @brief Opens the TCP connection.
     *
     * If the connection is already open, this function performs no operation.
     * Otherwise, this function attempts to connect to the address given during
     * construction.
     *
     * @throw std::system_error Operating system was unable to open the
     * connection.
     */
    void open();

    /**
     * @brief Terminates the TCP connection.
     *
     * If the connection is already closed, this function performs no operation.
     * Otherwise, the connection is gracefully shutdown and associated resources
     * are freed.
     *
     * @throw std::system_error Operating system was unable to close the
     * connection.
     */
    void terminate();

    /**
     * @brief Disables sending data over the TCP connection.
     *
     * This may be necessary in client-server interactions when the server needs
     * to know when the client is done sending information.
     *
     * @throw std::system_error Operating system was unable to perform partial
     * shutdown on the socket to disable sending.
     */
    void disable_send();

    /**
     * @brief Disables receiving data from the TCP connection.
     *
     * @throw std::system_error Operating system was unable to perform partial
     * shutdown on the socket to disable receiving.
     */
    void disable_receive();

    /**
     * @brief Checks if the TCP connection is active.
     * @return `true` if the connection is active, `false` otherwise.
     */
    [[nodiscard]] bool active() const;
};

class InactiveConnectionError : public std::exception {
   private:
    std::string _message;

   public:
    InactiveConnectionError(const std::string& prefix);
    InactiveConnectionError(const char* prefix);
    InactiveConnectionError(const InactiveConnectionError& other) = default;

    const char* what() const noexcept override;
};

#endif  // SOCKET_IMPL_H
