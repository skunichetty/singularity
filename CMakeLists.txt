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

message(STATUS "Using system \"${CMAKE_SYSTEM_NAME}\", using flags: ${CMAKE_CXX_FLAGS}")

enable_testing()
add_subdirectory(singularity)