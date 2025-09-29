#ifndef CLIENT_H
#define CLIENT_H

#define EVENT_SIZE (sizeof(struct inotify_event))
#define RETRY_COUNT 4
#define SOCKET_PATH_MAX 108
#define SOCKET_NAME "waypipe-daemon.sock"

char *get_socket_directory(void);
char *get_socket_path(void);
int connect_to_daemon(const char *path);
int wait_inotify(const char *socket_directory);
int start_daemon(void);

#endif // CLIENT_H
