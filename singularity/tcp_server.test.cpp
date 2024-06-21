#include "tcp_server.hpp"

#include <gtest/gtest.h>

#include <array>
#include <thread>
#include <vector>

#include "concurrency.hpp"

using namespace singularity;

constexpr size_t PORT = 10202;

TEST(TCPServerTest, BasicConnectivityTest) {
    network::TCPServer server(PORT);
    concurrency::FixedBuffer<network::TCPConnection, 30> connection_buffer;

    server.start(connection_buffer);
    server.shutdown();
}