#pragma once
#ifndef SOCKET_IMPL_H
#define SOCKET_IMPL_H

#include <netinet/in.h>
#include <sys/types.h>

#include <cstdint>
#include <cstddef>
#include <string>
#include <optional>

using socket_t = int;

/**
 * @brief Socket address wrapper;
 *
 * The `SocketAddress` class provides a representation of a socket address.
 * It encapsulates the underlying `sockaddr_storage` structure and provides
 * constant methods to access the address family, length, and data of the
 * socket address.
 */
class SocketAddress {
   protected:
    sockaddr_storage _address;
    SocketAddress() = default;

   public:
    [[nodiscard]] sa_family_t sa_family() const;
    [[nodiscard]] socklen_t length() const;
    [[nodiscard]] const sockaddr* data() const;
};

/**
 * @brief Represents an IPv4 socket address.
 *
 * The `IPSocketAddress` class is a subclass of `SocketAddress` and represents
 * an IP socket address. It provides methods to retrieve the IP address and port
 * associated with the IP socket address.
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
};

class MessageBuffer{
   private:
    std::unique_ptr<std::byte> _data;
    size_t _length;

   public:
    MessageBuffer(void* data, size_t datasize);
    [[nodiscard]] const std::byte* raw() const;
    [[nodiscard]] size_t length() const;
};

class TCPConnection {
   private:
    std::optional<socket_t> _socket;
    IPSocketAddress _address;

   public:
    TCPConnection(socket_t sock_fd, const IPSocketAddress& client_address);
    TCPConnection(const IPSocketAddress& server_address);

    void send_message(const MessageBuffer& buffer);
    MessageBuffer receive_message();
    
    void open();
    void terminate();

    [[nodiscard]] bool active() const;
};

#endif  // SOCKET_IMPL_H
