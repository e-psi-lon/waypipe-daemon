#include "logging.h"
#include <syslog.h>
#include <stdarg.h>
#include <stdbool.h>

static bool g_logging_initialized = false;

/**
 * Single logging function - called by macros
 */
void log_impl(int level, const char *fmt, ...) {
    if (!g_logging_initialized) {
        openlog("waypipe-client", LOG_PID | LOG_PERROR, LOG_USER);
        g_logging_initialized = true;
    }
    va_list args;
    va_start(args, fmt);
    vsyslog(level, fmt, args);
    va_end(args);
}
