#ifndef LOGGING_H
#define LOGGING_H
#include <syslog.h>

/**
 * Logging utility using syslog.
 * Auto-initializes on first use.
 *
 * Usage: log_info("Message: %s", value);
 *        log_err("Error code: %d", errno);
 *        log_warning("Warning");
 *        log_debug("Debug info");
 */

void log_impl(int level, const char *fmt, ...);

/**
 * Logging macros - wrap the implementation function
 * Uses syslog priority levels: LOG_INFO, LOG_ERR, LOG_WARNING, LOG_DEBUG
 */
#define log_info(fmt, ...) log_impl(LOG_INFO, fmt, ##__VA_ARGS__)
#define log_err(fmt, ...) log_impl(LOG_ERR, fmt, ##__VA_ARGS__)
#define log_warning(fmt, ...) log_impl(LOG_WARNING, fmt, ##__VA_ARGS__)
#define log_debug(fmt, ...) log_impl(LOG_DEBUG, fmt, ##__VA_ARGS__)

#endif // LOGGING_H

