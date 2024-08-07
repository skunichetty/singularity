include(GoogleTest)

add_executable(sockimpl_test sockimpl.test.cpp ${SRC_DIR}/sockimpl.cpp)
target_link_libraries(sockimpl_test GTest::gtest_main)

add_executable(
    tcp_server_test
    tcp_server.test.cpp
    ${SRC_DIR}/tcp_server.cpp
    ${SRC_DIR}/sockimpl.cpp
)
target_link_libraries(tcp_server_test GTest::gtest_main)


add_executable(concurrency_test concurrency.test.cpp)
target_link_libraries(concurrency_test GTest::gtest_main)

gtest_discover_tests(sockimpl_test)
gtest_discover_tests(tcp_server_test)
gtest_discover_tests(concurrency_test)