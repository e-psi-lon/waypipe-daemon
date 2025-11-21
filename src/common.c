#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "common.h"
#include "protocol.h"
#include "logging.h"

void close_ptr(const int *fd) {
    if (*fd >= 0) close(*fd);
}

void free_ptr(void **ptr) {
    if (*ptr) free(*ptr);
}

void free_message_ptr(message_t **msg) {
    free_message(*msg);
}


int get_socket_directory(char *buffer, const size_t size) {
    const char *xdg_runtime_dir = getenv("XDG_RUNTIME_DIR");
    if (xdg_runtime_dir && xdg_runtime_dir[0] != '\0') {
        if (snprintf(buffer, size, "%s", xdg_runtime_dir) >= (int)size)
            return EXIT_FAILURE;
    } else return EXIT_FAILURE;
    return EXIT_SUCCESS;
}

int get_socket_path(char *buffer, const size_t size, const char *dir) {
    if (!dir) {
        log_err("XDG_RUNTIME_DIR not set");
        return EXIT_FAILURE;
    }
    const int written = snprintf(buffer, size, "%s/%s", dir, DAEMON_INT_SOCK);
    if (written < 0 || (size_t)written >= size) {
        log_err("Socket path exceeds maximum length");
        return EXIT_FAILURE;
    }
    log_debug("Socket path: %s", buffer);
    return EXIT_SUCCESS;
}
