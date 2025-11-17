#ifndef WAYPIPEDAEMON_PROTOCOL_H
#define WAYPIPEDAEMON_PROTOCOL_H
#include <stddef.h>
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
void get_message_type_string(message_type_t type, char *buf, size_t buf_size);
message_t *create_message(message_type_t type, const char *data, size_t length);
message_t *read_message(int sockfd);
int send_message(int sockfd, const message_t *msg);
void free_message(message_t *msg);
void free_messages(message_t **msgs, size_t count);

#endif //WAYPIPEDAEMON_PROTOCOL_H