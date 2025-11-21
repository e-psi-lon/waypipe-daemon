#include "common.h"

#include <stdlib.h>
#include <unistd.h>

void close_ptr(const int *fd) {
    if (*fd >= 0) close(*fd);
}

void free_ptr(void **ptr) {
    if (*ptr) free(*ptr);
}

void free_message_ptr(message_t **msg) {
    free_message(*msg);
}