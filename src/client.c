#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/inotify.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include "client.h"

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
            fprintf(stderr, "Daemon socket not found after restart");
            return EXIT_FAILURE;
        }
        printf("Daemon started.\n");
    }
    printf("Waypipe client started.\n");
    // Then send the request to the daemon

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
    static char path[PATH_MAX];
    snprintf(path, sizeof(path), "/run/user/%d/", uid);
    path[sizeof(path) - 1] = '\0';
    return path;
}

/**
 * get_socket_path - Get the socket file path for the current user.
 *
 *
 * @return The socket file path for the current user
 */
char *get_socket_path(void) {
    static char filepath[PATH_MAX];
    const uid_t uid = getuid();
    snprintf(filepath, sizeof(filepath), "/run/user/%d/%s", uid, SOCKET_NAME);
    filepath[sizeof(filepath) - 1] = '\0';
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
    addr.sun_path[sizeof(addr.sun_path) - 1] = '\0';
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
