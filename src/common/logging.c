#include "logging.h"
#include <syslog.h>
#include <stdarg.h>
#include <stdatomic.h>
#include <stdbool.h>

static atomic_bool g_logging_initialized = ATOMIC_VAR_INIT(false);

/**
 * Weak default implementation - returns "waypipe"
 * Override where needed with a strong symbol
 */
weak const char *get_log_name(void) {
    return "wd-default";
}

/**
 * Weak default implementation - returns LOG_LOCAL0
 * Override where needed with a strong symbol
 */
weak int get_log_facility(void) {
    return LOG_LOCAL0;
}

void openlog_name(const char *name) {
    openlog(name, LOG_PID | LOG_PERROR, get_log_facility());
}

void vlog_impl(const int level, const char *fmt, va_list args) {
    bool expected = false;
    if (atomic_compare_exchange_strong(&g_logging_initialized, &expected, true)) {
        openlog_name(get_log_name());
    }
    va_list args_copy;
    va_copy(args_copy, args);
    vsyslog(level, fmt, args_copy);
    va_end(args_copy);
}


void log_impl(const int level, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vlog_impl(level, fmt, args);
    va_end(args);
}


