#include "protocol.h"
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <stdio.h>


void get_message_type_string(const message_type_t type, char *buf, size_t buf_size) {
    const char *name = NULL;
    switch (type) {
        case MSG_HELLO: name = "MSG_HELLO"; break;
        case MSG_READY: name = "MSG_READY"; break;
        case MSG_SEND: name = "MSG_SEND"; break;
        case MSG_RESPONSE_OK: name = "MSG_RESPONSE_OK"; break;
        case MSG_RESPONSE_ERROR: name = "MSG_RESPONSE_ERROR"; break;
        default: name = NULL; break;
    }
    if (name) {
        snprintf(buf, buf_size, "%s", name);
    } else {
        snprintf(buf, buf_size, "MSG_UNKNOWN(%u)", (unsigned)type);
    }
}

message_t *create_message(const message_type_t type, const char *data, size_t length) {
    message_t *msg = malloc(sizeof(message_header_t) + length);
    if (!msg) return NULL;
    msg->header.type = type;
    msg->header.length = length;
    if (data && length > 0) {
        memcpy(msg->data, data, length);
    }
    return msg;
}

int read_message(int sockfd, message_t **msgs) {
    return EXIT_SUCCESS;
}

int send_message(int sockfd, message_t *msg) {
    return EXIT_SUCCESS;
}

void free_message(message_t *msg) {
    if (!msg) return;
    free(msg);
}

void free_messages(message_t **msgs, const size_t count) {
    if (!msgs) return;
    for (size_t i = 0; i < count; i++) free_message(msgs[i]);
}