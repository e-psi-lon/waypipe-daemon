#ifndef WAYPIPEDAEMON_CLIENT_H
#define WAYPIPEDAEMON_CLIENT_H
#include <sys/inotify.h>

#define EVENT_SIZE (sizeof(struct inotify_event))
#define RETRY_COUNT 4


int connect_to_daemon(const char *path);
int wait_inotify(const char *socket_directory);
int start_daemon(void);
int fail(const char *msg, ...);
char *client_get_socket_path(void);
char *client_get_socket_directory(void);

#endif // WAYPIPEDAEMON_CLIENT_H
