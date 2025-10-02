cmake_minimum_required(VERSION 4.0)
project(WaypipeDaemon C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_executable(wdaemon src/daemon.c
        src/protocol.h
        src/protocol.c)
add_executable(wdclient src/client.c
        src/client.h
        src/protocol.h
        src/protocol.c)
