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

if(NOT MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror -Wconversion")
endif()

if(CMAKE_CXX_COMPILE_ID STREQUAL "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++ -fsanitize=integer")
endif()

add_executable(singularity singularity/main.cpp)

enable_testing()

add_executable(sockimpl_test singularity/sockimpl.test.cpp singularity/sockimpl.cpp)
target_include_directories(sockimpl_test PRIVATE singularity)
target_link_libraries(sockimpl_test GTest::gtest_main)

add_executable(concurrency_test singularity/concurrency.test.cpp)
target_include_directories(concurrency_test PRIVATE singularity)
target_link_libraries(concurrency_test GTest::gtest_main)


include(GoogleTest)
gtest_discover_tests(concurrency_test)
gtest_discover_tests(sockimpl_test)