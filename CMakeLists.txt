# FILEPATH: /home/caviar/TCP_IO/CMakeLists.txt
cmake_minimum_required(VERSION 3.10)

project(TCP_IO VERSION 1.0)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_compile_options(-Wall -Wextra -pedantic)

add_executable(client client.c)
add_executable(server_select server_select.c)
add_executable(server_epoll server_epoll.c)
add_executable(server_epoll_thread server_epoll_thread.c)

target_link_libraries(server_select pthread)
target_link_libraries(server_epoll_thread pthread)
