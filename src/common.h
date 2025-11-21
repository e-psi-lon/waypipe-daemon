#ifndef WAYPIPEDAEMON_COMMON_H
#define WAYPIPEDAEMON_COMMON_H
#include <sys/un.h>
#include "protocol.h"

#define SOCKET_PATH_MAX (sizeof(struct sockaddr_un) - sizeof(sa_family_t))
#define DAEMON_INT_SOCK "waypipe-daemon.sock"
#define STANDARD_BUFFER_SIZE 1024
#if defined(__GNUC__) || defined(__clang__)
    void close_ptr(const int *fd);
    void free_ptr(void **ptr);
    void free_message_ptr(message_t **msg);
    #define auto_close __attribute__((cleanup(close_ptr)))
    #define auto_free __attribute__((cleanup(free_ptr)))
    #define auto_free_message __attribute__((cleanup(free_message_ptr)))
#else
    #error "This code requires GCC or Clang"
#endif

/**
 * Get the socket directory path for the current user.
 * Uses XDG_RUNTIME_DIR environment variable to get the said directory
 *
 * @param buffer Buffer to store the directory path
 * @param size Size of the buffer
 * @return EXIT_SUCCESS on success, EXIT_FAILURE on failure
 */
int get_socket_directory(char *buffer, size_t size);

/**
 * Get the socket file path for the current user.
 *
 * @param buffer Buffer to store the socket file path
 * @param size Size of the buffer
 * @param dir Directory path
 * @return EXIT_SUCCESS on success, EXIT_FAILURE on failure
 */
int get_socket_path(char *buffer, size_t size, const char *dir);

#endif //WAYPIPEDAEMON_COMMON_H