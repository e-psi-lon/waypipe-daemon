#include <stdlib.h>
#include <unistd.h>
#include <sys/syslog.h>

#include "common/logging.h"

int main(void) {
    openlog_name("waypipe-daemon");
    log_debug("Starting Python daemon");

    char *pwd = getcwd(NULL, 0);
    log_debug("Current working directory: %s", pwd);
    closelog();

    return system("python3 ../daemon.py --send-ready");
}
