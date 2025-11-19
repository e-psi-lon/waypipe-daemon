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
#include "protocol.h"
#include "logging.h"

int main(const int argc, char *argv[]) {
    const char *socket_directory = get_socket_directory();
    const char *socket_path = get_socket_path();
    int sockfd = connect_to_daemon(socket_path);

    if (sockfd < 0) {
        log_info("Daemon not running. Starting daemon...");
        // Starts the daemon if not running
        start_daemon();
        const int status = wait_inotify(socket_directory);
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
        message_t *hello_msg = create_message(MSG_HELLO, NULL, 0);
        if (!hello_msg) {
            fail(sockfd, "Failed to create HELLO message");
        }
        if (send_message(sockfd, hello_msg) != EXIT_SUCCESS) {
            free_message(hello_msg);
            fail(sockfd, "Failed to send HELLO message");
        }
        free_message(hello_msg);
    }
    log_info("Waypipe daemon's client started");
    // Await for a READY message from the daemon
    message_t *response = read_message(sockfd);
    if (!response) {
        fail(sockfd, "Failed to read message from daemon");
    }
    if (response->header.type != MSG_READY) {
        char buf[32];
        get_message_type_string(response->header.type, buf, sizeof(buf));
        free_message(response);
        fail(sockfd, "Unexpected message type from daemon: %s", buf);
    }
    free_message(response);
    log_info("Connected to daemon successfully.");

    char argument_string_buf[4096];
    argument_string_buf[0] = '\0';
    size_t used = 0;
    int ret = 0;
    for (int i = 1; i < argc; i++) {
        ret = snprintf(argument_string_buf + used, sizeof(argument_string_buf) - used, "%s%s", argv[i], (i < argc - 1) ? " " : "");

        if (ret < 0 || (size_t)ret >= sizeof(argument_string_buf) - used)
            fail(sockfd, "Command line arguments too long");
        used += ret;
    }

    message_t *command = create_message(MSG_SEND, argument_string_buf, strlen(argument_string_buf));
    if (!command) {
        fail(sockfd, "Failed to create command message");
    }
    if (send_message(sockfd, command) != EXIT_SUCCESS){
        free_message(command);
        fail(sockfd, "Failed to send command message");
    }
    free_message(command);
    log_info("Command sent to daemon: \"%s\"", argument_string_buf);
    message_t *success_response = read_message(sockfd);
    if (!success_response) {
        fail(sockfd, "Failed to read response from daemon");
    }
    if (success_response->header.type != MSG_RESPONSE_OK) {
        fail(sockfd, "Daemon didn't receive the command successfully: %s", success_response->data);
    }
    log_info("Command received by the daemon successfully.");
    free_message(success_response);
    close(sockfd);
    closelog();
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
    snprintf(filepath, sizeof(filepath), "/run/user/%d/%s", uid, DAEMON_INT_SOCK);
    log_debug("Socket path: %s", filepath);
    return filepath;
}

_Noreturn void fail(const int sockfd, const char *msg, ...) {
    va_list ap;
    va_start(ap, msg);
    vlog_err(msg, ap);
    va_end(ap);
    if (sockfd >= 0)
        close(sockfd);
    else
        log_err("Attempted to close an invalid socket descriptor.");
    closelog();
    exit(EXIT_FAILURE);
};


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
            if (event->len > 0 && (event->mask & IN_CREATE) && strcmp(event->name, DAEMON_INT_SOCK) == 0) {
                log_info("Daemon socket created. Connecting...");
                socket_found = true;
                break;
            }
            i += EVENT_SIZE + event->len;
        }
    }
    inotify_rm_watch(inotify_fd, watch_fd);
    close(inotify_fd);
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

    // Initialize logging in child process BEFORE redirecting streams
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
    char client_path[1024];
    const ssize_t len = readlink("/proc/self/exe", client_path, sizeof(client_path) - 1);
    if (len < 0) {
        log_err("Failed to resolve client executable path");
        exit(EXIT_FAILURE);
    }
    client_path[len] = '\0';
    log_debug("Resolved client executable path: %s", client_path);

    char *dir = dirname(client_path);
    if (!dir) {
        log_err("Failed to extract directory from path");
        exit(EXIT_FAILURE);
    }
    log_debug("Extracted directory: %s", dir);

    char daemon_path[1024];
    snprintf(daemon_path, sizeof(daemon_path), "%s/wdaemon", dir);
    log_debug("Daemon executable path: %s", daemon_path);

    log_debug("Executing daemon: %s", daemon_path);
    execv(daemon_path, (char *const[]){ "wdaemon", NULL });

    // If execv returns, something went wrong
    exit(EXIT_FAILURE);
}
