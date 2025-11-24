#include <stdlib.h>
#include <unistd.h>
#include <sys/syslog.h>

#include "common/logging.h"

// Logging configuration (overrides weak symbols from logging.c)
const char *get_log_name(void) {
    return "wdaemon";
}

int get_log_facility(void) {
    return LOG_DAEMON;
}


int main(void) {
    log_debug("Starting Python daemon");

    char *pwd = getcwd(NULL, 0);
    log_debug("Current working directory: %s", pwd);
    closelog();

    return system("python3 ../daemon.py --send-ready");
}
