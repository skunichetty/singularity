include_directories(include)
# note that this is different from the in-built cmake var
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

add_executable(main ${SRC_DIR}/main.cpp)
add_subdirectory(test)
