add_executable(
    server_performance_loopback
    server_performance_loopback.cpp
    ${SRC_DIR}/sockimpl.cpp
    ${SRC_DIR}/tcp_server.cpp
)
add_executable(buffer_performance buffer_performance.cpp)