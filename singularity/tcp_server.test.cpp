#include "tcp_server.hpp"

#include <gtest/gtest.h>

#include <array>
#include <iostream>
#include <thread>
#include <vector>

#include "concurrency.hpp"
#include "sockimpl.hpp"

using namespace singularity;

constexpr uint16_t PORT = 10202;

class TCPServerTest : public testing::Test {
   protected:
    concurrency::DynamicBuffer<bool> client_states;
    std::atomic<size_t> num_clients = 0;
    std::mutex cerr_mutex;
    std::atomic<bool> handler_shutdown = false;

    void launch_loopback_client() {
        size_t current_client = num_clients++;
        network::IPSocketAddress server_address("127.0.0.1", PORT);
        network::TCPConnection client(server_address);

        auto message = network::MessageBuffer::from_string(
            "hello from client " + std::to_string(current_client) + "!");

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
            client_states.push(out == message);
        } catch (std::exception& e) {
            std::unique_lock<std::mutex> lock(cerr_mutex);
            std::cerr << "Error in client " << current_client << ": "
                      << e.what() << std::endl;
            exit(1);
        }
    }

    void connection_handler(concurrency::FixedBuffer<network::TCPConnection,
                                                     30>& connection_buffer) {
        while (!handler_shutdown) {
            auto ctx = connection_buffer.pop(std::chrono::milliseconds(50));
            if (ctx.has_value()) {
                auto out = ctx->receive_message();
                {
                    std::unique_lock<std::mutex> lock(cerr_mutex);
                    std::cerr << "Server received message: "
                              << reinterpret_cast<const char*>(out.raw())
                              << "\n";
                }
                ctx->send_message(out);
            }
        }
    }
};

TEST_F(TCPServerTest, BasicConnectivityTest) {
    network::TCPServer server(PORT);
    concurrency::FixedBuffer<network::TCPConnection, 30> connection_buffer;

    server.start(connection_buffer);
    server.shutdown();
}

TEST_F(TCPServerTest, SingleConnectionLoopbackTest) {
    network::TCPServer server(PORT);
    concurrency::FixedBuffer<network::TCPConnection, 30> connection_buffer;

    std::thread handle_thread([this, &connection_buffer]() {
        connection_handler(connection_buffer);
    });

    server.start(connection_buffer);
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    launch_loopback_client();
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    server.shutdown();

    handler_shutdown.exchange(true);
    handle_thread.join();

    while (!client_states.empty()) {
        EXPECT_TRUE(client_states.pop());
    }
}

TEST_F(TCPServerTest, MultiConnectionLoopbackTest) {
    network::TCPServer server(PORT);
    concurrency::FixedBuffer<network::TCPConnection, 30> connection_buffer;

    std::thread handle_thread([this, &connection_buffer]() {
        connection_handler(connection_buffer);
    });

    server.start(connection_buffer);
    std::this_thread::sleep_for(std::chrono::milliseconds(1));

    std::vector<std::thread> backing(10);
    for (size_t index = 0; index < 10; ++index) {
        backing.emplace_back([this]() { launch_loopback_client(); });
    }
    for (auto& thread : backing) {
        if (thread.joinable()) thread.join();
    }
    server.shutdown();
    handler_shutdown.exchange(true);
    handle_thread.join();

    while (!client_states.empty()) {
        EXPECT_TRUE(client_states.pop());
    }
}
