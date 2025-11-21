#include "protocol.h"
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <stdio.h>
#include <sys/socket.h>

#include "logging.h"


void get_message_type_string(const message_type_t type, char *buf, const size_t buf_size) {
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

message_t *create_message(const message_type_t type, const char *data, const size_t length) {
    if (!!data != (length > 0)) return NULL;
    if (length > MAX_MESSAGE_SIZE) return NULL;
    if (data && data[length - 1] != '\0') return NULL;
    message_t *msg = malloc(sizeof(message_header_t) + length);
    if (!msg) return NULL;
    msg->header.type = type;
    msg->header.length = length;
    if (data) memcpy(msg->data, data, length);
    log_debug("Allocating message with type: %" PRIu32, msg->header.type);
    return msg;
}

message_t *read_message(const int sockfd) {
    message_header_t header;
    ssize_t length = recv(sockfd, &header, sizeof(message_header_t), MSG_WAITALL);
    if (length < 0) {
        perror("recv");
        return NULL;
    }
    if (length < sizeof(message_header_t)){
        log_err("Incomplete message header");
        return NULL;
    }
    message_t *msg = malloc(sizeof(message_header_t) + header.length);
    if (!msg) {
        perror("malloc");
        return NULL;
    }
    msg->header = header;
    if (header.length > 0) {
        if (header.length > MAX_MESSAGE_SIZE) {
            free_message(msg);
            log_err("Message data received exceeds maximum size");
            return NULL;
        }
        length = recv(sockfd, msg->data, header.length, MSG_WAITALL);
        if (length < 0) {
            perror("recv");
            free_message(msg);
            return NULL;
        }
        if (length < header.length) {
            free_message(msg);
            log_err("Incomplete message data");
            return NULL;
        }
    }
    return msg;
}

int send_message(const int sockfd, const message_t *msg) {
    ssize_t length = send(sockfd, &msg->header, sizeof(message_header_t), MSG_NOSIGNAL);
    if (length < 0) {
        perror("send");
        return EXIT_FAILURE;
    }
    if (length < sizeof(message_header_t)) {
        log_err("Incomplete message header sent");
        return EXIT_FAILURE;
    }
    // If data is present
    if (msg->header.length > 0) {
        length = send(sockfd, msg->data, msg->header.length, MSG_NOSIGNAL);
        if (length < 0) {
            perror("send");
            return EXIT_FAILURE;
        }
        if (length < msg->header.length) {
            log_err("Incomplete message data sent");
            return EXIT_FAILURE;
        }
    }
    return EXIT_SUCCESS;
}

void free_message(message_t *msg) {
    if (!msg) return;
    log_debug("Freeing message with type: %" PRIu32, msg->header.type);
    free(msg);
}

void free_messages(message_t **msgs, const size_t count) {
    if (!msgs) return;
    for (size_t i = 0; i < count; i++) free_message(msgs[i]);
}