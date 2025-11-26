#include <stdbool.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/inotify.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <libgen.h>
#include "client.h"
#include <limits.h>
#include "common/common.h"
#include "common/protocol.h"
#include "common/logging.h"


// Logging configuration (overrides weak symbols from logging.c)
const char *get_log_name(void) {
    return "wdclient";
}

int get_log_facility(void) {
    return LOG_USER;
}


int main(const int argc, char *argv[]) {
    if (argc < 2) {
        return fail("Missing command to execute\nUsage: %s <command...>", argv[0]);
    }
    const char *socket_directory = client_get_socket_directory();
    const char *socket_path = client_get_socket_path();
    if (!socket_path) {
        return fail("Failed to get socket path");
    }
    auto_close int sockfd = connect_to_daemon(socket_path);

    if (sockfd < 0) {
        log_info("Daemon not running. Starting daemon...");
        // Starts the daemon if not running
        // The daemon "knows" it's starting, it automatically
        // Sends a READY message when initialized
        if (start_daemon() != EXIT_SUCCESS)
            return fail("Failed to start daemon");
        int status;
        // The socket might have created the socket too fast
        // Ensure it doesn't exist before waiting for inotify
        if (access(socket_path, F_OK)) status = wait_inotify(socket_directory);
        else status = EXIT_SUCCESS;
        if (status != EXIT_SUCCESS)
            return fail("Failed to check for daemon socket");
        int attempts = 0;
        const int max_attempts = 5;
        int delay_ms = 50;

        while (attempts < max_attempts) {
            sockfd = connect_to_daemon(socket_path);
            if (sockfd >= 0) break;

            log_debug("Connection attempt %d failed, retrying in %dms",
                      attempts + 1, delay_ms);
            usleep((useconds_t)delay_ms * 1000);
            delay_ms *= 2;
            attempts++;
        }
        if (sockfd < 0) {
            return fail("Daemon socket not found after restart");
        }
        log_info("Daemon started.");
    } else {
        log_info("Connecting to existing daemon...");
        // Alert the daemon that a new client has connected
        // The daemon then acknowledges the message reception with a READY
        auto_free_message message_t *hello_msg = create_message(MSG_HELLO, NULL, 0);
        if (!hello_msg)
            return fail("Failed to create HELLO message");
        if (send_message(sockfd, hello_msg) != EXIT_SUCCESS)
            return fail("Failed to send HELLO message");
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

    // 4096 characters is more than enough for 99.999% GUI app commands.
    // 1024 or 2048 could've been used, but this ensures 0.099% use case coverage
    // Without being too expensive
    char argument_string_buf[STANDARD_BUFFER_SIZE * 4];
    argument_string_buf[0] = '\0';
    size_t used = 0;
    int ret = 0;
    for (int i = 1; i < argc; i++) {
        ret = snprintf(argument_string_buf + used, sizeof(argument_string_buf) - used, "%s%s", argv[i],
                       i < argc - 1 ? " " : "");

        if (ret < 0)
            return fail("Encoding error in command line arguments");
        if ((size_t)ret >= sizeof(argument_string_buf) - used)
            return fail("Command line arguments too long");
        used += (size_t)ret;
    }

    auto_free_message message_t* command = create_message(MSG_SEND, argument_string_buf,
                                                          STRLENGTH_WITH_NULL(argument_string_buf));
    if (!command) {
        return fail("Failed to create command message");
    }
    if (send_message(sockfd, command) != EXIT_SUCCESS) {
        return fail("Failed to send command message");
    }
    log_info("Command sent to daemon: \"%s\"", argument_string_buf);
    auto_free_message message_t *success_response = read_message(sockfd);
    if (!success_response)
        return fail("Failed to read success response from daemon");
    if (success_response->header.type != MSG_RESPONSE_OK)
        return fail("Daemon didn't receive the command successfully: %s",
                    success_response->header.length > 0 ? success_response->data : "(no message in error response)");
    log_info("Command received by the daemon successfully.");
    closelog();
    return EXIT_SUCCESS;
}

char *client_get_socket_directory(void) {
    static char socket_directory[SOCKET_PATH_MAX];
    static bool initialized = false;
    if (initialized) return socket_directory;
    if (get_socket_directory(socket_directory, sizeof(socket_directory)) != EXIT_SUCCESS)
        return NULL;
    initialized = true;
    return socket_directory;
}

char *client_get_socket_path(void) {
    static char socket_path[SOCKET_PATH_MAX];
    static bool initialized = false;
    if (initialized) return socket_path;
    const char *socket_directory = client_get_socket_directory();
    if (!socket_directory) return NULL;
    if (get_socket_path(socket_path, sizeof(socket_path), socket_directory) != EXIT_SUCCESS)
        return NULL;
    initialized = true;
    return socket_path;
}

int fail(const char *msg, ...) {
    va_list ap;
    va_start(ap, msg);
    vlog_err(msg, ap);
    va_end(ap);
    closelog();
    return EXIT_FAILURE;
}


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
    char event_buf[EVENT_SIZE * 64];
    bool socket_found = false;
    for (int retry = 0; retry <= MESSAGE_RECV_RETRIES*2 && !socket_found; retry++) {
        struct pollfd pfd = {
            .fd = inotify_fd,
            .events = POLLIN
        };
        const int poll_ret = poll(&pfd, 1, MESSAGE_RECV_TIMEOUT_MS);
        if (poll_ret < 0) {
            perror("poll");
            continue;
        }
        if (poll_ret == 0) {
            log_debug("Inotify poll timeout, retrying... (%d/%d)", retry + 1, MESSAGE_RECV_RETRIES*2);
            continue;
        }
        const ssize_t length = read(inotify_fd, event_buf, sizeof(event_buf));
        if (length < 0) {
            perror("read");
            if (inotify_rm_watch(inotify_fd, watch_fd) < 0) {
                log_debug("inotify_rm_watch failed"); // Despite being a system error, we only log it as debug since it's non-fatal
            }
            return EXIT_FAILURE;
        }
        for (ssize_t i = 0; i < length;) {
            struct inotify_event event;
            memcpy(&event, &event_buf[i], EVENT_SIZE);
            // This is required to avoid reading from uninitialized memory
            const char *name_ptr = &event_buf[i + (ssize_t)EVENT_SIZE];
            if (event.len > 0 && event.mask & IN_CREATE && strncmp(name_ptr, DAEMON_INT_SOCK, event.len) == 0) {
                log_info("Daemon socket created. Connecting...");
                socket_found = true;
                break;
            }
            i += (ssize_t)EVENT_SIZE + event.len;
        }
    }
    if (inotify_rm_watch(inotify_fd, watch_fd) < 0) {
        log_debug("inotify_rm_watch failed"); // Same as above, non-fatal
    }
    if (!socket_found) {
        log_err("Daemon socket not found after %d retries.", MESSAGE_RECV_RETRIES*2);
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
        int status;
        const pid_t wpid = waitpid(pid, &status, 0);
        if (wpid < 0) {
            perror("waitpid");
            return EXIT_FAILURE;
        }
        if (!WIFEXITED(status) || WEXITSTATUS(status) != EXIT_SUCCESS) {
            log_err("Daemon failed to start");
            return EXIT_FAILURE;
        }
        log_info("Daemon started successfully");
        return EXIT_SUCCESS;
    }

    // Child process: prepare environment and exec the daemon
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
    openlog_name("wdaemon-launcher");
    char client_path[PATH_MAX];
    const ssize_t len = readlink("/proc/self/exe", client_path, sizeof(client_path) - 1);
    if (len < 0) {
        log_err("Failed to resolve client executable path");
        exit(EXIT_FAILURE);
    }
    client_path[len] = '\0';
    log_debug("Resolved client executable path: %s", client_path);

    char path_copy[PATH_MAX];
    snprintf(path_copy, sizeof(path_copy), "%s", client_path);
    char *dir = dirname(path_copy);
    if (!dir) {
        log_err("Failed to extract directory from path");
        exit(EXIT_FAILURE);
    }
    log_debug("Extracted directory: %s", dir);
    char daemon_path[PATH_MAX];
    snprintf(daemon_path, sizeof(daemon_path), "%s/wdaemon", dir);
    log_debug("Daemon executable path: %s", daemon_path);
    if (access(daemon_path, X_OK)) {
        log_err("Daemon executable not found or not executable: %s", daemon_path);
        exit(EXIT_FAILURE);
    }

    log_debug("Executing daemon: %s", daemon_path);
    execv(daemon_path, (char *const[]){"wdaemon", NULL});
    // If execv returns, something went wrong
    perror("execv");
    exit(EXIT_FAILURE);
}
