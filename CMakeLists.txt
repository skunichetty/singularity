cmake_minimum_required(VERSION 3.14)
project(singularity LANGUAGES CXX)

if (POLICY CMP0077)
    cmake_policy(SET CMP0077 NEW)
endif (POLICY CMP0077)

if (POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif (POLICY CMP0135)

message(STATUS "Starting configuration process in directory ${CMAKE_CURRENT_SOURCE_DIR}")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/build")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_FLAGS "-Wall")

# Fetch requisite dependencies
include(FetchContent)
FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

FetchContent_Declare(
    openblas
    URL https://github.com/OpenMathLib/OpenBLAS/archive/refs/tags/v0.3.27.tar.gz
    URL_HASH MD5=ef71c66ffeb1ab0f306a37de07d2667f 
)

# Set OpenBLAS configure options
set(BUILD_TESTING OFF)
set(BUILD_LAPACK_DEPRECATED OFF)
set(BUILD_STATIC_LIBS ON)
set(ONLY_CBLAS ON)

FetchContent_MakeAvailable(googletest openblas)

# Resolve system specific options
if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror -Wconversion -pthread")
elseif (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror -Wconversion")
endif ()

if (CMAKE_CXX_COMPILE_ID STREQUAL "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
endif ()

message(STATUS "Found system \"${CMAKE_SYSTEM_NAME}\", using flags: ${CMAKE_CXX_FLAGS} ")

add_executable(singularity singularity/main.cpp)
target_link_libraries(singularity openblas_static)
target_include_directories(singularity PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/generated")
target_include_directories(singularity PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")

enable_testing()

add_executable(sockimpl_test singularity/sockimpl.test.cpp singularity/sockimpl.cpp)
target_include_directories(sockimpl_test PRIVATE singularity)
target_link_libraries(sockimpl_test GTest::gtest_main)

add_executable(concurrency_test singularity/concurrency.test.cpp)
target_include_directories(concurrency_test PRIVATE singularity)
target_link_libraries(concurrency_test GTest::gtest_main)

add_executable(buffer_performance test/buffer_performance.cpp)
target_include_directories(buffer_performance PRIVATE singularity)

add_executable(tcp_server_test
        singularity/tcp_server.test.cpp
        singularity/tcp_server.cpp
        singularity/sockimpl.cpp)
target_include_directories(tcp_server_test PRIVATE singularity)
target_link_libraries(tcp_server_test GTest::gtest_main)

add_executable(server_performance_loopback
        test/server_performance_loopback.cpp
        singularity/sockimpl.cpp
        singularity/tcp_server.cpp)
target_include_directories(server_performance_loopback PRIVATE singularity)

include(GoogleTest)
gtest_discover_tests(concurrency_test)
gtest_discover_tests(sockimpl_test)
gtest_discover_tests(tcp_server_test)

