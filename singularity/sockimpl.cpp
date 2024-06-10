#include "sockimpl.hpp"

#include <arpa/inet.h>
#include <unistd.h>

#include <cstring>
#include <memory>
#include <system_error>

#include "utils.hpp"

constexpr size_t MIN_BUFFER_SIZE = 256;
constexpr size_t BUFFER_EPSILON = 32;


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
    char buffer[INET_ADDRSTRLEN] = {0};
    const char* str = inet_ntop(AF_INET, &(_ip_addr->sin_addr), buffer, INET_ADDRSTRLEN);
    if(str == NULL){
        throw std::system_error(errno, std::system_category(), "Unable to construct string IP address");
    }
    return std::string(buffer);
}

uint16_t IPSocketAddress::port() const { return ntohs(_ip_addr->sin_port); }

MessageBuffer::MessageBuffer(void* data, size_t datasize) : _length{datasize} {
    _data.reset(new std::byte[datasize]);
    memcpy(_data.get(), data, datasize);
}

const std::byte* MessageBuffer::raw() const {
    return _data.get();
}

size_t MessageBuffer::length() const {
    return _length;
}

TCPConnection::TCPConnection(socket_t sock_fd, const IPSocketAddress& client_address): _socket{sock_fd}, _address{client_address} {}

TCPConnection::TCPConnection(const IPSocketAddress& server_address) : _socket{std::nullopt}, _address{server_address} {}

void TCPConnection::open() {
    if(!_socket.has_value()){
        _socket = socket(AF_INET, SOCK_STREAM, 0);
        int status = connect(*_socket, _address.data(), _address.length());
        if(status == -1) {
            throw std::system_error(
                errno,
                std::system_category(),
                "Unable to open TCP connection"
            );
        }
    }
}

void TCPConnection::terminate() {
    if(_socket.has_value()){
        close(*_socket);
        _socket.reset();
    }
}

bool TCPConnection::active() const {
    return _socket.has_value();
}

void TCPConnection::send_message(const MessageBuffer& buffer){
    const std::byte* next_byte = buffer.raw();
    size_t remaining_bytes = buffer.length();
    ssize_t bytes_sent = 0;

    do {
        bytes_sent = send(*_socket, next_byte, remaining_bytes, 0);

        if(bytes_sent == -1){
            throw std::system_error(
                errno,
                std::system_category(),
                "Failure to send message"
            );
        } 

        auto offset = static_cast<size_t>(bytes_sent);
        remaining_bytes -= offset;
        next_byte += offset; 
    } while(remaining_bytes > 0);
}

MessageBuffer TCPConnection::receive_message() {
    auto buffer = std::make_unique<std::byte[]>(MIN_BUFFER_SIZE);
    size_t buffer_capacity = MIN_BUFFER_SIZE;

    std::byte* next_byte = buffer.get();
    size_t bytes_written = 0;
    ssize_t bytes_received = 0;
    
    do{
        bytes_received = recv(*_socket, next_byte, MIN_BUFFER_SIZE - bytes_written, 0);

        if(bytes_received == -1){
            throw std::system_error(
                errno,
                std::system_category(),
                "Error in receiving message"
            );
        }

        auto offset = static_cast<size_t>(bytes_received);
        bytes_written += offset;
        next_byte += offset;

        if(bytes_written + BUFFER_EPSILON >= buffer_capacity){
            // grow buffer eagerly in anticipation of more data

            // NOTE: we can dynamically tune epsilon in response 
            // to stream patterns for performance. Not necessary atm though.
            auto copy = std::make_unique<std::byte[]>(buffer_capacity * 2);
            memcpy(copy.get(), buffer.get(), bytes_written);
            buffer.swap(copy);
            buffer_capacity *= 2;
        }
    } while (bytes_received != 0);

    return MessageBuffer(buffer.get(), bytes_written);
}
