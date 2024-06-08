#include "sockimpl.hpp"

#include <gtest/gtest.h>
#include <netinet/in.h>

#include <limits>

#include "arpa/inet.h"

TEST(IPSocketAddressImplTest, IPCharAddressCreation) {
    IPSocketAddress addr("127.0.0.1", 3000);
    EXPECT_STREQ(addr.address_str().c_str(), "127.0.0.1");

    EXPECT_THROW(
        { IPSocketAddress addr("asdjflkjsd", 3000); }, std::invalid_argument);
    EXPECT_THROW(
        { IPSocketAddress addr("432.423.432.432", 3000); },
        std::invalid_argument);
}

TEST(IPSocketAddressImplTest, IPPortConversions) {
    IPSocketAddress addr(INADDR_ANY, 3000);
    EXPECT_EQ(addr.port(), 3000);
}

TEST(IPSocketAddressImplTest, IPCharAddressConversion) {
    IPSocketAddress addr("127.0.0.1", 3000);
    uint32_t intaddr = addr.address();
    in_addr temp;
    temp.s_addr = htonl(intaddr);
    char* straddr = inet_ntoa(temp);
    EXPECT_STREQ(straddr, addr.address_str().c_str());
}