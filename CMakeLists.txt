cmake_minimum_required(VERSION 4.0)
project(WaypipeDaemon C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_library(wdcommon OBJECT
        src/common/protocol.h
        src/common/protocol.c
        src/common/logging.h
        src/common/logging.c
        src/common/common.c
        src/common/common.h
)

add_executable(wdaemon src/daemon/daemon.c
        src/daemon/daemon.h)
target_link_libraries(wdaemon PRIVATE wdcommon)
add_executable(wdclient src/client/client.c
        src/client/client.h)
target_link_libraries(wdclient PRIVATE wdcommon)
