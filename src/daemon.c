#include <stdlib.h>
#include <unistd.h>
#include <sys/syslog.h>

#include "logging.h"

int main(void) {
    openlog("waypipe-daemon", LOG_PID, LOG_DAEMON);
    log_debug("Starting Python daemon");

    char *pwd = getcwd(NULL, 0);
    log_debug("Current working directory: %s", pwd);
    closelog();

    return system("python3 ../daemon.py --send-ready");
}
