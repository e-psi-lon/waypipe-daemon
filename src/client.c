#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/inotify.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>
#include "client.h"
#include "protocol.h"

int main(int argc, char *argv[]) {
    const char *socket_directory = get_socket_directory();
    const char *socket_path = get_socket_path();
    int sockfd = connect_to_daemon(socket_path);

    if (sockfd < 0) {
        printf("Daemon not running. Starting daemon...\n");
        // Starts the daemon if not running
        start_daemon();
        const int status = wait_inotify(socket_directory);
        if (status != EXIT_SUCCESS) {
            return EXIT_FAILURE;
        }
        sockfd = connect_to_daemon(socket_path);
        if (sockfd < 0) {
            fprintf(stderr, "Daemon socket not found after restart\n");
            return EXIT_FAILURE;
        }
        printf("Daemon started.\n");
    } else {
        printf("Connecting to existing daemon...\n");
        // Alert the daemon that a new client has connected
        message_t *hello_msg = create_message(MSG_HELLO, NULL, 0);
        if (!hello_msg) {
            fprintf(stderr, "Failed to create HELLO message\n");
            close(sockfd);
            return EXIT_FAILURE;
        }
        if (send_message(sockfd, hello_msg) != EXIT_SUCCESS) {
            fprintf(stderr, "Failed to send HELLO message\n");
            free_message(hello_msg);
            close(sockfd);
            return EXIT_FAILURE;
        }
        free_message(hello_msg);
    }
    printf("Waypipe client started.\n");
    // Await for a READY message from the daemon
    message_t *response = NULL;
    if (read_message(sockfd, &response) != EXIT_SUCCESS) {
        fprintf(stderr, "Failed to read message from daemon\n");
        close(sockfd);
        return EXIT_FAILURE;
    }
    if (response->header.type != MSG_READY) {
        char buf[32];
        get_message_type_string(response->header.type, buf, sizeof(buf));
        fprintf(stderr, "Unexpected message type from daemon: %s\n", buf);
        free_message(response);
        close(sockfd);
        return EXIT_FAILURE;
    }
    free_message(response);
    printf("Connected to daemon successfully.\n");

    close(sockfd);
    return EXIT_SUCCESS;
}

/**
 * get_socket_directory - Get the socket directory path for the current user.
 *
 * @return The socket directory path for the current user.
 */
char *get_socket_directory(void) {
    const uid_t uid = getuid();
    static char path[SOCKET_PATH_MAX];
    snprintf(path, sizeof(path), "/run/user/%d/", uid);
    return path;
}

/**
 * get_socket_path - Get the socket file path for the current user.
 *
 *
 * @return The socket file path for the current user
 */
char *get_socket_path(void) {
    static char filepath[SOCKET_PATH_MAX];
    const uid_t uid = getuid();
    snprintf(filepath, sizeof(filepath), "/run/user/%d/%s", uid, SOCKET_NAME);
    return filepath;
}

int connect_to_daemon(const char *path) {
    const int sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return -1;
    }
    struct sockaddr_un addr;
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, path, sizeof(addr.sun_path) - 1);
    if (connect(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("connect");
        close(sockfd);
        return -1;
    }
    return sockfd;
}

int wait_inotify(const char *socket_directory) {
    const int inotify_fd = inotify_init1(IN_CLOEXEC);
    if (inotify_fd < 0) {
        perror("inotify_init1");
        return EXIT_FAILURE;
    }
    // Create the file path:
    const int watch_fd = inotify_add_watch(inotify_fd, socket_directory, IN_CREATE);
    if (watch_fd < 0) {
        perror("inotify_add_watch");
        close(inotify_fd);
        return EXIT_FAILURE;
    }
    char event_buf[EVENT_SIZE*64];
    bool socket_found = false;
    for (int retry = 0; retry < RETRY_COUNT && !socket_found; retry++) {
        const ssize_t length = read(inotify_fd, event_buf, sizeof(event_buf));
        if (length < 0) {
            perror("read");
            inotify_rm_watch(inotify_fd, watch_fd);
            close(inotify_fd);
            return EXIT_FAILURE;
        }
        for (size_t i = 0; i < length;) {
            const struct inotify_event *event = (const struct inotify_event *)&event_buf[i];
            if (event->len > 0 && (event->mask & IN_CREATE) && strcmp(event->name, SOCKET_NAME) == 0) {
                printf("Daemon socket created. Connecting...\n");
                socket_found = true;
                break;
            }
            i += EVENT_SIZE + event->len;
        }
    }
    inotify_rm_watch(inotify_fd, watch_fd);
    close(inotify_fd);
    if (!socket_found) {
        fprintf(stderr, "Daemon socket not found after %d retries.\n", RETRY_COUNT);
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;

}

int start_daemon(void) {
    // This is a placeholder for starting the daemon process.
    printf("Starting daemon process...\n");
    return 0;
}
