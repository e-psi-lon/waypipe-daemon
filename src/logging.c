#include "logging.h"
#include <syslog.h>
#include <stdarg.h>
#include <stdbool.h>

static bool g_logging_initialized = false;


void vlog_impl(const int level, const char *fmt, va_list args) {
    if (!g_logging_initialized) {
        openlog("waypipe-client", LOG_PID | LOG_PERROR, LOG_USER);
        g_logging_initialized = true;
    }
    vsyslog(level, fmt, args);
}


void log_impl(const int level, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vlog_impl(level, fmt, args);
    va_end(args);
}


