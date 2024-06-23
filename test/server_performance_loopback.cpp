#include <fcntl.h>
#include <unistd.h>

#include <atomic>
#include <cassert>
#include <iostream>
#include <mutex>
#include <thread>

#include "tcp_server.hpp"

constexpr size_t TOTAL_CONNECTIONS = 10000;
constexpr size_t NUM_THREADS = 15;
constexpr uint16_t PORT = 10202;

using namespace singularity;

void launch_client(std::atomic<size_t>& num_connections, size_t ctx_count) {
    int urandom = open("/dev/urandom", O_RDONLY);
    for (size_t ctx_index = 0; ctx_index < ctx_count; ++ctx_index) {
        size_t current_client = num_connections++;
        network::IPSocketAddress server_address("127.0.0.1", PORT);
        network::TCPConnection client(server_address);

        constexpr size_t num_data = 43283;
        char data[num_data];
        ssize_t bytes_read = read(urandom, data, num_data);
        if (bytes_read < 0) {
            std::cerr << strerror(errno) << std::endl;
            exit(1);
        }
        network::MessageBuffer message(data, static_cast<size_t>(bytes_read));

        try {
            client.open();
        } catch (std::exception& e) {
            std::cerr << e.what() << std::endl;
            exit(1);
        }

        try {
            client.send_message(message);
            client.disable_send();
            auto out = client.receive_message();
            assert(out == message);
        } catch (std::exception& e) {
            std::cerr << "Error in client " << current_client << ": "
                      << e.what() << std::endl;
            exit(1);
        }
    }
}

int main() {
    std::atomic<size_t> num_connections = 0;
    std::atomic<bool> handler_shutdown = false;

    size_t num_connections_per_thread = (TOTAL_CONNECTIONS / NUM_THREADS);
    size_t last_amount =
        TOTAL_CONNECTIONS - num_connections_per_thread * (NUM_THREADS - 1);

    network::TCPServer server(PORT);
    concurrency::FixedBuffer<network::TCPConnection, 30> connection_buffer;

    auto handler = [&handler_shutdown, &connection_buffer]() {
        while (!handler_shutdown) {
            auto ctx = connection_buffer.pop(std::chrono::milliseconds(50));
            if (ctx.has_value()) {
                auto out = ctx->receive_message();
                ctx->send_message(out);
            }
        }
    };
    std::thread handle_thread(handler);

    server.start(connection_buffer);
    std::this_thread::sleep_for(std::chrono::milliseconds(1));

    std::vector<std::thread> backing(NUM_THREADS);
    for (size_t index = 0; index < NUM_THREADS; ++index) {
        size_t ctx_count = (index == NUM_THREADS - 1)
                               ? last_amount
                               : num_connections_per_thread;
        backing.emplace_back(launch_client, std::ref(num_connections),
                             ctx_count);
    }

    for (auto& thread : backing) {
        if (thread.joinable()) thread.join();
    }

    server.shutdown();
    handler_shutdown.exchange(true);
    handle_thread.join();
}