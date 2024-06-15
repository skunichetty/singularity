#include "sockimpl.hpp"

#include <fcntl.h>
#include <gtest/gtest.h>
#include <netinet/in.h>

#include <atomic>
#include <chrono>
#include <limits>
#include <thread>

#include "arpa/inet.h"

constexpr uint16_t PORT = 32322;

// Test fixture that creates and runs a server that accepts a fixed number of
// connections and waits for acceptance.
class TCPConnectionTest : public testing::Test {
   protected:
    int _socket;
    std::mutex m;
    std::condition_variable connection_signal;
    std::optional<std::thread> runner = std::nullopt;

    static constexpr size_t BUFFER_SIZE = 50000;

    TCPConnectionTest() {
        _socket = socket(AF_INET, SOCK_STREAM, 0);

        int reuse = 1;
        setsockopt(_socket, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

        sockaddr_in addr{};
        addr.sin_port = htons(PORT);
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_family = AF_INET;
        memset(addr.sin_zero, 0, 8 * sizeof(char));

        bind(_socket, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
        listen(_socket, 1);
    }

    void start_server(int num_connections) {
        runner = std::thread([this, num_connections]() {
            for (size_t index = 0; index < num_connections; ++index) {
                sockaddr_in addr;
                socklen_t addrlen = sizeof(addr);

                connection_signal.notify_one();
                int client_socket = accept(
                    _socket, reinterpret_cast<sockaddr*>(&addr), &addrlen);

                char buffer[BUFFER_SIZE];
                ssize_t bytes_recv = 0;
                size_t total_bytes_recv = 0;
                do {
                    bytes_recv = recv(client_socket, buffer + total_bytes_recv,
                                      BUFFER_SIZE - total_bytes_recv, 0);
                    if (bytes_recv == -1) ASSERT_TRUE(false);
                    total_bytes_recv += static_cast<size_t>(bytes_recv);
                } while (bytes_recv != 0 && total_bytes_recv < BUFFER_SIZE);

                ssize_t bytes_sent = 0;
                size_t total_bytes_sent = 0;
                do {
                    bytes_sent = send(client_socket, buffer + total_bytes_sent,
                                      total_bytes_recv - total_bytes_sent, 0);
                    if (bytes_sent == -1) ASSERT_TRUE(false);
                    total_bytes_sent += static_cast<size_t>(bytes_sent);
                } while (total_bytes_sent < total_bytes_recv);

                // remember to close the socket!
                close(client_socket);
            }
        });

        // wait until _right_ before accept syscall
        std::unique_lock<std::mutex> lock(m);
        connection_signal.wait(lock);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    void TearDown() override {
        if (runner.has_value()) {
            runner->join();
        }
        close(_socket);
    }
};

TEST(IPSocketAddressImplTest, IPCharAddressCreation) {
    IPSocketAddress addr("127.0.0.1", 3000);
    EXPECT_STREQ(addr.address_str().c_str(), "127.0.0.1");

    EXPECT_THROW(
        { IPSocketAddress addr1("asdjflkjsd", 3000); }, std::invalid_argument);
    EXPECT_THROW(
        { IPSocketAddress addr2("432.423.432.432", 3000); },
        std::invalid_argument);
}

TEST(IPSocketAddressImplTest, IPPortConversions) {
    IPSocketAddress addr(INADDR_ANY, 3000);
    EXPECT_EQ(addr.port(), 3000);
}

TEST(IPSocketAddressImplTest, IPCharAddressConversion) {
    IPSocketAddress addr("178.234.41.239", 3000);

    uint32_t intaddr = addr.address();
    in_addr temp;
    temp.s_addr = htonl(intaddr);
    char* straddr = inet_ntoa(temp);

    EXPECT_STREQ(straddr, addr.address_str().c_str());
}

TEST_F(TCPConnectionTest, ClientBasicLoopbackTest) {
    TCPConnection connection(IPSocketAddress("127.0.0.1", PORT));
    auto buffer = MessageBuffer::from_string("hi! sending from connection");

    start_server(1);

    try {
        connection.open();
    } catch (std::system_error& error) {
        std::cerr << error.what() << std::endl;
        exit(1);
    }

    connection.send_message(buffer);
    connection.disable_send();  // disable writes, trigger server response

    auto out = connection.receive_message();
    EXPECT_STREQ(reinterpret_cast<const char*>(buffer.raw()),
                 reinterpret_cast<const char*>(out.raw()));
}

TEST_F(TCPConnectionTest, ClientInvalidServer) {
    TCPConnection connection(IPSocketAddress("127.0.0.1", PORT - 1));
    EXPECT_THROW({ connection.open(); }, std::system_error);
}

TEST_F(TCPConnectionTest, ConnectionInvalidState) {
    TCPConnection connection(IPSocketAddress("127.0.0.1", PORT));
    auto buffer = MessageBuffer::from_string("this will fail!");

    EXPECT_THROW({ connection.send_message(buffer); }, InactiveConnectionError);
    EXPECT_THROW({ connection.receive_message(); }, InactiveConnectionError);
}

TEST_F(TCPConnectionTest, ClientStateConfirmation) {
    TCPConnection connection(IPSocketAddress("127.0.0.1", PORT));
    EXPECT_FALSE(connection.active());

    start_server(1);

    try {
        connection.open();
    } catch (std::system_error& error) {
        std::cerr << error.what() << std::endl;
        exit(1);
    }

    EXPECT_TRUE(connection.active());
    connection.disable_send();
    EXPECT_TRUE(connection.active());
    connection.terminate();
    EXPECT_FALSE(connection.active());
}

TEST_F(TCPConnectionTest, ClientSideBufferTest) {
    int urandom = open("/dev/urandom", O_RDONLY);

    constexpr size_t num_data = 43283;
    char data[num_data];
    read(urandom, data, num_data);
    MessageBuffer buffer(data, num_data);

    TCPConnection connection(IPSocketAddress("127.0.0.1", PORT));

    start_server(1);

    try {
        connection.open();
    } catch (std::system_error& error) {
        std::cerr << error.what() << std::endl;
        exit(1);
    }

    connection.send_message(buffer);
    connection.disable_send();

    auto out = connection.receive_message();
    connection.terminate();

    EXPECT_EQ(out.length(), buffer.length());
    for (size_t index = 0; index < out.length(); ++index) {
        EXPECT_EQ(out.raw()[index], buffer.raw()[index]);
    }
}