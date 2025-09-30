#ifndef WAYPIPEDAEMON_PROTOCOL_H
#define WAYPIPEDAEMON_PROTOCOL_H
#include <stdint.h>

typedef struct {
    uint32_t type;
    uint32_t length;
} message_header_t;

typedef enum {
    MSG_HELLO = 1,
    MSG_READY = 2,
    MSG_SEND = 3,
    MSG_RESPONSE_OK = 100,
    MSG_RESPONSE_ERROR = 101
} message_type_t;

typedef struct {
    message_header_t header;
    char data[];  // Variable payload
} message_t;


#endif //WAYPIPEDAEMON_PROTOCOL_H