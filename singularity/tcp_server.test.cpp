#include "tcp_server.hpp"

#include <gtest/gtest.h>

#include <limits>

TEST(TCPServerTest, TCPServerPortConstruction) {
    EXPECT_THROW({ TCPServer server(23487293); }, std::invalid_argument);
    EXPECT_NO_THROW({ TCPServer server(0); });
    uint32_t max_port =
        static_cast<uint32_t>(std::numeric_limits<uint16_t>::max());
    EXPECT_NO_THROW({ TCPServer server(max_port); });
}
