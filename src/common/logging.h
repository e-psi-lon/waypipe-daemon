#ifndef LOGGING_H
#define LOGGING_H
#include <stdarg.h>
#include <syslog.h>
#include "common.h"

/**
 * Logging utility using syslog.
 * Auto-initializes on first use with per-binary name and facility.
 *
 * The logging system uses weak symbols to allow each binary to customize
 * its log name and facility. Define strong versions of get_log_name() and
 * get_log_facility() in your binary to override the defaults.
 *
 * Usage: log_info("Message: %s", value);
 *        log_err("Error code: %d", errno);
 *        log_warning("Warning");
 *        log_debug("Debug info");
 */

/**
 * Get the log name for this binary.
 * Override this function in your binary to customize the syslog identity.
 * Default: "waypipe"
 *
 * @return The name to use for syslog identification
 */
const char *get_log_name(void);

/**
 * Get the syslog facility for this binary.
 * Override this function in your binary to customize the facility.
 * Default: LOG_LOCAL0
 *
 * @return The syslog facility (e.g., LOG_USER, LOG_DAEMON)
 */
int get_log_facility(void);

void openlog_name(const char *name);

void vlog_impl(int level, const char *fmt, va_list args) format_func(printf, 2, 0);
void log_impl(int level, const char *fmt, ...) format_func(printf, 2, 3);

/**
 * Logging macros - wrap the implementation function
 * Uses syslog priority levels: LOG_INFO, LOG_ERR, LOG_WARNING, LOG_DEBUG
 */
#define log_info(fmt, ...) log_impl(LOG_INFO, fmt, ##__VA_ARGS__)
#define log_err(fmt, ...) log_impl(LOG_ERR, fmt, ##__VA_ARGS__)
#define log_warning(fmt, ...) log_impl(LOG_WARNING, fmt, ##__VA_ARGS__)
#define log_debug(fmt, ...) log_impl(LOG_DEBUG, fmt, ##__VA_ARGS__)
#define vlog_info(fmt, args) vlog_impl(LOG_INFO, fmt, args)
#define vlog_err(fmt, args) vlog_impl(LOG_ERR, fmt, args)
#define vlog_warning(fmt, args) vlog_impl(LOG_WARNING, fmt, args)
#define vlog_debug(fmt, args) vlog_impl(LOG_DEBUG, fmt, args)

#endif // LOGGING_H

