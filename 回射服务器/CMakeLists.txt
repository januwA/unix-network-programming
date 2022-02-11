cmake_minimum_required(VERSION 3.0.0)
project(echo_server VERSION 0.1.0)

include(CTest)
enable_testing()

add_executable(server server.c)
add_executable(client client.c)
add_executable(t t.c)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
