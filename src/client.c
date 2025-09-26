#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>


int connect_to_daemon(void) {
    int sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return -1;
    }
    struct sockaddr_un addr;
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, "/var/run/waypipe-daemon.sock", sizeof(addr.sun_path) - 1);
    addr.sun_path[sizeof(addr.sun_path) - 1] = '\0';
    if (connect(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(sockfd);
        return -1;
    }
    return sockfd;
}

int main(int argc, char *argv[]) {
    int sockfd = connect_to_daemon();
    if (sockfd < 0) {
        printf("Daemon not running. Starting daemon...\n");
        // Starts the daemon if not running
    }
    printf("Waypipe client started.\n");
    // Then send the request to the daemon

    close(sockfd);
    return 0;
}
