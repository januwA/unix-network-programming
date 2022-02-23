cmake_minimum_required(VERSION 3.0.0)
project(echo_server VERSION 0.1.0)

include(CTest)
enable_testing()

# use c++ 20
set(CMAKE_CXX_STANDARD 20)

add_executable(server server.cpp sock.cpp)
add_executable(client client.cpp sock.cpp)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
