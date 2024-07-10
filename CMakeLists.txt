cmake_minimum_required(VERSION 3.14)
project(singularity LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/build")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_FLAGS "-Wall")

# Fetch requisite dependencies
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Fetch requisite dependencies
include(FetchContent)
FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Find and link OpenBLAS
find_package(
    OpenBLAS_CMAKE
    REQUIRED
    NAMES OpenBLAS
    PATHS /opt/OpenBLAS/lib/cmake)
if(OpenBLAS_CMAKE_FOUND)
    message(STATUS "OpenBLAS Version: ${OpenBLAS_VERSION}")
    message(STATUS "OpenBLAS Library: ${OpenBLAS_LIBRARIES}")
    message(STATUS "OpenBLAS Include Directory: ${OpenBLAS_INCLUDE_DIRS}")
endif()

# Resolve system specific options
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror -Wconversion -pthread")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror -Wconversion")
endif()

if (CMAKE_CXX_COMPILE_ID STREQUAL "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
endif ()

message(STATUS "Found system \"${CMAKE_SYSTEM_NAME}\", using flags: ${CMAKE_CXX_FLAGS}")

add_executable(singularity singularity/main.cpp)
target_include_directories(singularity PRIVATE ${OpenBLAS_INCLUDE_DIRS})
target_link_libraries(singularity ${OpenBLAS_LIBRARIES})

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

