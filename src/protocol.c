#include "protocol.h"
#include <stdlib.h>

int read_message(int sockfd, message_t **msg) {
    return EXIT_SUCCESS;
}

int send_message(int sockfd, message_t *msg) {
    return EXIT_SUCCESS;
}

void free_message(message_t *msg) {
    free(msg->data);
    free(msg);
}