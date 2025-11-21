#include <stdbool.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/inotify.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>
#include <fcntl.h>
#include <libgen.h>
#include "client.h"
#include "common.h"
#include "protocol.h"
#include "logging.h"

int main(const int argc, char *argv[]) {
    const char *socket_directory = client_get_socket_directory();
    const char *socket_path = client_get_socket_path();
    if (!socket_path) {
        log_err("Failed to get socket path");
        return EXIT_FAILURE;
    }
    auto_close int sockfd = connect_to_daemon(socket_path);

    if (sockfd < 0) {
        log_info("Daemon not running. Starting daemon...");
        // Starts the daemon if not running
        start_daemon();
        int status;
        // The socket might have created the socket too fast
        if (access(socket_path, F_OK)) status = wait_inotify(socket_directory);
        else status = EXIT_SUCCESS;
        if (status != EXIT_SUCCESS) {
            return EXIT_FAILURE;
        }
        sockfd = connect_to_daemon(socket_path);
        if (sockfd < 0) {
            log_err("Daemon socket not found after restart");
            return EXIT_FAILURE;
        }
        log_info("Daemon started.");
    } else {
        log_info("Connecting to existing daemon...");
        // Alert the daemon that a new client has connected
        auto_free_message message_t *hello_msg = create_message(MSG_HELLO, NULL, 0);
        if (!hello_msg) {
            return fail("Failed to create HELLO message");
        }
        if (send_message(sockfd, hello_msg) != EXIT_SUCCESS) {
            return fail("Failed to send HELLO message");
        }
    }
    log_info("Waypipe daemon's client started");
    // Await for a READY message from the daemon
    auto_free_message message_t *response = read_message(sockfd);
    if (!response) {
        return fail("Failed to read message from daemon");
    }
    if (response->header.type != MSG_READY) {
        char buf[32];
        get_message_type_string(response->header.type, buf, sizeof(buf));
        return fail("Unexpected message type from daemon: %s", buf);
    }
    log_info("Connected to daemon successfully.");

    char argument_string_buf[STANDARD_BUFFER_SIZE*4];
    argument_string_buf[0] = '\0';
    size_t used = 0;
    int ret = 0;
    for (int i = 1; i < argc; i++) {
        ret = snprintf(argument_string_buf + used, sizeof(argument_string_buf) - used, "%s%s", argv[i], (i < argc - 1) ? " " : "");

        if (ret < 0 || (size_t)ret >= sizeof(argument_string_buf) - used)
            return fail("Command line arguments too long");
        used += ret;
    }

    auto_free_message message_t *command = create_message(MSG_SEND, argument_string_buf, STRLENGTH_WITH_NULL(argument_string_buf));
    if (!command) {
        return fail("Failed to create command message");
    }
    if (send_message(sockfd, command) != EXIT_SUCCESS){
        return fail("Failed to send command message");
    }
    log_info("Command sent to daemon: \"%s\"", argument_string_buf);
    auto_free_message message_t *success_response = read_message(sockfd);
    if (!success_response) {
        return fail("Failed to read success response from daemon");
    }
    if (success_response->header.type != MSG_RESPONSE_OK) {
        return fail("Daemon didn't receive the command successfully: %s",
                    success_response->data[0] ? success_response->data : "(no message in error response)");
    }
    log_info("Command received by the daemon successfully.");
    closelog();
    return EXIT_SUCCESS;
}

char *client_get_socket_directory(void) {
    static char socket_directory[SOCKET_PATH_MAX];
    if (get_socket_directory(socket_directory, sizeof(socket_directory)) != EXIT_SUCCESS)
        return NULL;
    return socket_directory;
}

char *client_get_socket_path(void) {
    static char socket_path[SOCKET_PATH_MAX];
    const char *socket_directory = client_get_socket_directory();
    if (!socket_directory) return NULL;
    if (get_socket_path(socket_path, sizeof(socket_path), socket_directory) != EXIT_SUCCESS)
        return NULL;
    return socket_path;
}

int fail(const char *msg, ...) {
    va_list ap;
    va_start(ap, msg);
    vlog_err(msg, ap);
    va_end(ap);
    closelog();
    return EXIT_FAILURE;
};


int connect_to_daemon(const char *path) {
    const int sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return -1;
    }
    struct sockaddr_un addr = {0};
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, path, SOCKET_PATH_MAX - 1);
    addr.sun_path[SOCKET_PATH_MAX - 1] = '\0';
    if (connect(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("connect");
        close(sockfd);
        return -1;
    }
    return sockfd;
}

int wait_inotify(const char *socket_directory) {
    const auto_close int inotify_fd = inotify_init1(IN_CLOEXEC);
    if (inotify_fd < 0) {
        perror("inotify_init1");
        return EXIT_FAILURE;
    }
    // Create the file path:
    const int watch_fd = inotify_add_watch(inotify_fd, socket_directory, IN_CREATE);
    if (watch_fd < 0) {
        perror("inotify_add_watch");
        return EXIT_FAILURE;
    }
    char event_buf[EVENT_SIZE*64];
    bool socket_found = false;
    for (int retry = 0; retry < RETRY_COUNT && !socket_found; retry++) {
        const ssize_t length = read(inotify_fd, event_buf, sizeof(event_buf));
        if (length < 0) {
            perror("read");
            inotify_rm_watch(inotify_fd, watch_fd);
            return EXIT_FAILURE;
        }
        for (size_t i = 0; i < length;) {
            const struct inotify_event *event = (const struct inotify_event *)&event_buf[i];
            if (event->len > 0 && (event->mask & IN_CREATE) && strcmp(event->name, DAEMON_INT_SOCK) == 0) {
                log_info("Daemon socket created. Connecting...");
                socket_found = true;
                break;
            }
            i += EVENT_SIZE + event->len;
        }
    }
    inotify_rm_watch(inotify_fd, watch_fd);
    if (!socket_found) {
        log_err("Daemon socket not found after %d retries.", RETRY_COUNT);
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;

}

int start_daemon(void) {
    log_info("Starting daemon process...");

    const pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return EXIT_FAILURE;
    }

    if (pid > 0) {
        log_debug("Parent process: daemon forked with PID %d", pid);
        return EXIT_SUCCESS;
    }

    // Initialize logging
    openlog_name("waypipe-daemon");
    log_debug("Child process: starting daemon initialization");

    const int devnull = open("/dev/null", O_RDWR);
    if (devnull < 0) {
        perror("open");
        exit(EXIT_FAILURE);
    }
    dup2(devnull, STDIN_FILENO);
    dup2(devnull, STDOUT_FILENO);
    dup2(devnull, STDERR_FILENO);
    if (devnull > 2) {
        close(devnull);
    }

    if (setsid() < 0) {
        log_err("Failed to create new session");
        exit(EXIT_FAILURE);
    }

    unsetenv("LD_PRELOAD");
    unsetenv("LD_LIBRARY_PATH");
    char client_path[STANDARD_BUFFER_SIZE];
    const ssize_t len = readlink("/proc/self/exe", client_path, sizeof(client_path) - 1);
    if (len < 0) {
        log_err("Failed to resolve client executable path");
        exit(EXIT_FAILURE);
    }
    client_path[len] = '\0';
    log_debug("Resolved client executable path: %s", client_path);

    char path_copy[STANDARD_BUFFER_SIZE];
    strncpy(path_copy, client_path, sizeof(path_copy));
    char *dir = dirname(path_copy);
    if (!dir) {
        log_err("Failed to extract directory from path");
        exit(EXIT_FAILURE);
    }
    log_debug("Extracted directory: %s", dir);

    char daemon_path[STANDARD_BUFFER_SIZE];
    snprintf(daemon_path, sizeof(daemon_path), "%s/wdaemon", dir);
    log_debug("Daemon executable path: %s", daemon_path);

    log_debug("Executing daemon: %s", daemon_path);
    execv(daemon_path, (char *const[]){ "wdaemon", NULL });

    // If execv returns, something went wrong
    exit(EXIT_FAILURE);
}
