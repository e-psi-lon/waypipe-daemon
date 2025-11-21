cmake_minimum_required(VERSION 4.0)
project(WaypipeDaemon C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_library(wdcommon OBJECT
        src/protocol.h
        src/protocol.c
        src/logging.h
        src/logging.c
)

add_executable(wdaemon src/daemon.c
        src/daemon.h)
target_link_libraries(wdaemon PRIVATE wdcommon)
add_executable(wdclient src/client.c
        src/client.h)
target_link_libraries(wdclient PRIVATE wdcommon)
