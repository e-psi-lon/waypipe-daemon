/**
* @file client.h
 * @brief Definitions of the functions used inside the client's code
 *
 * This file defines the functions used by the WaypipeDaemon client
 * to connect to the daemon, handle errors, and manage socket paths.
 * The main() function uses all these functions to implement the client's logic.
 */

#ifndef WAYPIPEDAEMON_CLIENT_H
#define WAYPIPEDAEMON_CLIENT_H
#include <sys/inotify.h>


/**
 * @brief Size of an inotify event structure
 *
 * This defines a shorthand to access the size of an event from inotify.
 */
#define EVENT_SIZE (sizeof(struct inotify_event))

/**
 * @brief Number of retries to connect to the socket before giving up
 *
 * This is used when the client tries to connect to the daemon's socket
 * after starting the daemon process. The client will attempt to connect
 * this many times, with a short delay between attempts, before giving up.
 */
#define RETRY_COUNT 4


/**
 * @brief Connect to the socket at a given path.
 *
 * @param path The path to the socket.
 * @return The socket file descriptor, or -1 on error
 */
int connect_to_daemon(const char *path);

/**
 * @brief Wait for the inotify even signaling the socket creation.
 **
 * @brief Wait for the inotify even signaling the socket creation.
 *
 * @param socket_directory The directory to watch for the socket creation.
 * @return 0 on success, -1 on error
 */
int wait_inotify(const char *socket_directory);

/**
 * @brief Start the daemon in a forked process
 *
 * Starts a subprocess that'll run the daemon.
 * The said daemon is responsible for the demonization.
 *
 * @return 0 on success, -1 on error
 */
int start_daemon(void);

/**
 * @brief Handle a fatal error from the main
 *
 * Format and log the given message, close the logger, and returns an exit code to use
 * from the main function.
 *
 * @param msg The message to log
 * @param ... Additional arguments for formatting the message
 * @return The exit code to use (EXIT_FAILURE)
 */
int fail(const char *msg, ...);

/**
 * @brief Get the path to the socket - adapted to the client.
 *
 * @note Adapted to the single-threaded environment of the client.
 * Do not attempt to use in a threaded environment.
 *
 * @return The socket path, or NULL on failure
 */
char *client_get_socket_path(void);

/**
 * @brief Get the directory path of the socket - adapted to the client.
 *
 * @return
 */
char *client_get_socket_directory(void);

#endif // WAYPIPEDAEMON_CLIENT_H
