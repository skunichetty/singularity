#include "sockimpl.hpp"

#include <arpa/inet.h>
#include <unistd.h>

#include <cstring>
#include <memory>
#include <system_error>

#include "utils.hpp"

static std::system_error get_syserr() {
    return std::system_error(errno, std::system_category(),
                             "Error starting TCP server");
}

sa_family_t SocketAddress::sa_family() const { return _address.ss_family; }
socklen_t SocketAddress::length() const { return sizeof(_address); }
const sockaddr* SocketAddress::data() const {
    return reinterpret_cast<const sockaddr*>(&_address);
}

IPSocketAddress::IPSocketAddress(uint32_t address, uint16_t port)
    : _ip_addr{reinterpret_cast<sockaddr_in*>(&_address)} {
    // note: should profile hton/ntoh operations for latency
    _ip_addr->sin_family = AF_INET;
    _ip_addr->sin_addr.s_addr = htonl(address);
    _ip_addr->sin_port = htons(port);
}

IPSocketAddress::IPSocketAddress(const char* address, uint16_t port)
    : _ip_addr{reinterpret_cast<sockaddr_in*>(&_address)} {
    _ip_addr->sin_family = AF_INET;
    _ip_addr->sin_port = htons(port);

    if (inet_aton(address, &(_ip_addr->sin_addr)) == 0) {
        std::string error_msg = build_string("Invalid address: ", address);
        throw std::invalid_argument(error_msg);
    }
}

IPSocketAddress::IPSocketAddress(const sockaddr* address, socklen_t addrlen) {
    memcpy(&_address, address, addrlen);
}

IPSocketAddress::IPSocketAddress(const IPSocketAddress& other)
    : _ip_addr{reinterpret_cast<sockaddr_in*>(&_address)} {}

IPSocketAddress& IPSocketAddress::operator=(const IPSocketAddress& other) {
    return (*this = IPSocketAddress(other));
}

uint32_t IPSocketAddress::address() const {
    return ntohl(_ip_addr->sin_addr.s_addr);
}

std::string IPSocketAddress::address_str() const {
    auto buffer = std::make_unique<char[]>(INET_ADDRSTRLEN);
    inet_ntop(AF_INET, &(_ip_addr->sin_addr.s_addr), buffer.get(),
              INET_ADDRSTRLEN);
    std::string string_address(buffer.get());
    return string_address;
}

uint16_t IPSocketAddress::port() const { return ntohs(_ip_addr->sin_port); }

TCPSocket::TCPSocket() : _socket{socket(AF_INET, SOCK_STREAM, 0)} {}

void TCPSocket::bind_address(const IPSocketAddress& info) {
    int bind_status = bind(_socket, info.data(), info.length());
    if (bind_status == -1) throw get_syserr();
}

void TCPSocket::start_listen(int max_connections) {
    int listen_status = listen(_socket, max_connections);
    if (listen_status == -1) throw get_syserr();
}

std::pair<socket_t, IPSocketAddress> TCPSocket::accept_ctx() {
    sockaddr_in client_addr;
    socklen_t addrlen = sizeof(client_addr);
    sockaddr* addr_ptr = reinterpret_cast<sockaddr*>(&client_addr);

    socket_t ctx = accept(_socket, addr_ptr, &addrlen);
    if (ctx == -1) throw get_syserr();

    return std::make_pair(ctx, IPSocketAddress(addr_ptr, addrlen));
}

void TCPSocket::disconnect() { close(_socket); }

TCPSocket::~TCPSocket() { close(_socket); }