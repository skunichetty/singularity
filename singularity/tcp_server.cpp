#include "tcp_server.hpp"

#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <atomic>
#include <limits>
#include <stdexcept>
#include <system_error>
#include <thread>

#include "poll.h"
#include "utils.hpp"

using namespace singularity::network;

constexpr static uint32_t MAX_PORT_NUM =
    static_cast<uint32_t>(std::numeric_limits<uint16_t>::max());
constexpr static size_t TIMEOUT = 50;

void throw_system_error(const std::string& message) {
    throw std::system_error(errno, std::system_category(), message);
}

class TCPServer::TCPServerImpl {
   public:
    std::atomic<bool> shutdown;
    uint16_t port;
    pollfd socket_info;

    std::optional<std::thread> main_thread;

    TCPServerImpl(uint32_t port) : shutdown{false}, main_thread{std::nullopt} {
        if (port > MAX_PORT_NUM) {
            std::string error_message = singularity::utils::build_string(
                "Invalid port number ", port, ", expected in range [0,",
                MAX_PORT_NUM, "]");
            throw std::invalid_argument(error_message);
        }
        port = static_cast<uint16_t>(port);
        socket_info.events = POLLIN;
    }

    void setup() {
        socket_t& sock_fd = socket_info.fd;
        sock_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (sock_fd == -1) {
            throw_system_error("Unable to allocate socket");
        }

        int reuse = 1;
        int option_status = setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR,
                                       &reuse, sizeof(reuse));
        if (option_status == -1) {
            throw_system_error("Cannot enable socket reuse");
        }

        IPSocketAddress address(INADDR_ANY, port);
        int bind_status = bind(sock_fd, address.data(), address.length());
        if (bind_status == -1) {
            throw_system_error("Unable to bind socket to given port");
        }

        // queue none - this is as connection queue is controlled at app level
        int listen_status = listen(sock_fd, 1);
        if (listen_status == -1) {
            throw_system_error("Unable to set socket to listen");
        }
    }

    void start(concurrency::Buffer<TCPConnection>& connection_buffer) {
        auto runner = [this, &connection_buffer]() {
            int timeout = TIMEOUT;  // can dynamically adjust this in future

            IPSocketAddress address;
            socklen_t address_length = address.length();

            while (!shutdown) {
                auto num_events = poll(&socket_info, 1, timeout);
                if (num_events > 0 && socket_info.revents | POLLIN) {
                    int client_socket =
                        accept(socket_info.fd, address.data(), &address_length);
                    connection_buffer.push(
                        TCPConnection(client_socket, address));
                }
            }
        };
        main_thread = std::thread(runner);
    }

    ~TCPServerImpl() {
        if (main_thread->joinable()) {
            main_thread->join();
        }
        close(socket_info.fd);
    }
};

TCPServer::TCPServer(uint32_t port) {
    impl = std::make_unique<TCPServerImpl>(port);
}

void TCPServer::start(concurrency::Buffer<TCPConnection>& connection_buffer) {
    impl->setup();
    impl->start(connection_buffer);
}

void TCPServer::shutdown() { impl->shutdown = true; }
TCPServer::~TCPServer() { shutdown(); };