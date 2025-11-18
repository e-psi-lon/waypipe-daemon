#ifndef CLIENT_H
#define CLIENT_H
#include <sys/inotify.h>

#define EVENT_SIZE (sizeof(struct inotify_event))
#define RETRY_COUNT 4
#define SOCKET_PATH_MAX 108
#define DAEMON_INT_SOCK "waypipe-daemon.sock"

char *get_socket_directory(void);
char *get_socket_path(void);
int connect_to_daemon(const char *path);
int wait_inotify(const char *socket_directory);
int start_daemon(void);
_Noreturn void fail(int sockfd, const char *msg, ...);

#endif // CLIENT_H
