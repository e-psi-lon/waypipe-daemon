cmake_minimum_required(VERSION 4.0)
project(WaypipeDaemon C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_CUDA_STANDARD_REQUIRED ON)

add_executable(WaypipeDaemon src/daemon.c)
add_executable(WaypipeDaemonClient src/client.c)
