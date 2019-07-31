cmake_minimum_required(VERSION 3.9)
project(noam C)

set(CMAKE_C_STANDARD 99)

add_executable(noam noam.c)
target_link_libraries(noam)