#include <stdio.h>
#include <sys/inotify.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>


#define EVENT_SIZE (sizeof(struct inotify_event))
#define RETRY_COUNT 5
#define PATH_MAX 108

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
    snprintf(filepath, sizeof(filepath), "/run/user/%d/waypipe-daemon.sock", uid);
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
    strcpy(addr.sun_path, path);
    if (connect(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("connect");
        close(sockfd);
        return -1;
    }
    return sockfd;
}

int start_daemon(void) {
    // This is a placeholder for starting the daemon process.
    printf("Starting daemon process...\n");
    return 0;
}

int main(int argc, char *argv[]) {
    const char *socket_directory = get_socket_directory();
    const char *socket_path = get_socket_path();
    const int sockfd = connect_to_daemon(socket_path);

    if (sockfd < 0) {
        printf("Daemon not running. Starting daemon...\n");
        // Starts the daemon if not running
        start_daemon();
        const int inotify_fd = inotify_init1(IN_CLOEXEC);
        // Create the file path:
        const int watch_fd = inotify_add_watch(inotify_fd, socket_directory, IN_CREATE);
        char event_buf[EVENT_SIZE*64];
        for (int retry = 0; retry < RETRY_COUNT; retry++) {
            const ssize_t length = read(inotify_fd, event_buf, sizeof(event_buf));
            if (length < 0) {
                perror("read");
                return 1;
            }
            for (size_t i = 0; i < length;) {
                const struct inotify_event *event = (const struct inotify_event *)&event_buf[i];
                if (event->len > 0 && (event->mask & IN_CREATE) && strcmp(event->name, "waypipe-daemon.sock") == 0) {
                    printf("Daemon socket created. Connecting...\n");
                    break;
                }
                i += EVENT_SIZE + event->len;
            }
        }
        inotify_rm_watch(inotify_fd, watch_fd);


        printf("Daemon started.\n");
    }
    printf("Waypipe client started.\n");
    // Then send the request to the daemon

    close(sockfd);
    return 0;
}
