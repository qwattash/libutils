#ifndef LOG_H
#define LOG_H

#include <stdlib.h>

/**
 * Log level
 */
enum log_level {
  LOG_DEBUG = 0,
  LOG_INFO = 1,
  LOG_WARNING = 2,
  LOG_ERROR = 3,
  LOG_MSG = 4,
  LOG_NONE = 5,
};

/**
 * Internal logger handle representation.
 * This is useful for static initialization of the logger.
 */
struct logger_handle {
  enum log_level level;
  const char *prefix;
};

/**
 * Shorthand logger handle type
 */
typedef struct logger_handle * logger_t;

/* private logging functions - use macros */
void _log_config(logger_t logger, enum log_level lvl, const char *prefix);
void _log_stdout(logger_t logger, enum log_level lvl,
		 const char *fmt, ...);
void _log_stderr(logger_t logger, enum log_level lvl,
		 const char *fmt, ...);
logger_t _logger_init(void);

#ifdef ENABLE_LOGGING

#ifdef LOG_NODEBUG
#define log_debug(fmt, ...)
#define xlog_debug(logger, fmt, ...)
#else /* ! LOG_NODEBUG */
#define log_debug(fmt, ...) _log_stdout(NULL, LOG_DEBUG, fmt, ##__VA_ARGS__)
#define xlog_debug(logger, fmt, ...)			\
  _log_stdout(logger, LOG_DEBUG, fmt, ##__VA_ARGS__)
#endif /* ! LOG_NODEBUG */

#define log_info(fmt, ...) _log_stdout(NULL, LOG_INFO, fmt, ##__VA_ARGS__)
#define log_warn(fmt, ...) _log_stdout(NULL, LOG_WARNING, fmt, ##__VA_ARGS__)
#define log_err(fmt, ...) _log_stderr(NULL, LOG_ERROR, fmt, ##__VA_ARGS__)
#define log_msg(fmt, ...) _log_stdout(NULL, LOG_MSG, fmt, ##__VA_ARGS__)

#define xlog_info(logger, fmt, ...)			\
  _log_stdout(logger, LOG_INFO, fmt, ##__VA_ARGS__)
#define xlog_warn(logger, fmt, ...)			\
  _log_stdout(logger, LOG_WARNING, fmt, ##__VA_ARGS__)
#define xlog_err(logger, fmt, ...)			\
  _log_stderr(logger, LOG_ERROR, fmt, ##__VA_ARGS__)
#define xlog_msg(logger, fmt, ...)			\
  _log_stdout(logger, LOG_MSG, fmt, ##__VA_ARGS__)

#define log_config(logger, lvl, prefix) _log_config(logger, lvl, prefix)

/* avoid warning for unused variables when disabling logging */
#define logger_init(name) name = _logger_init()
#define logger_free(name) free(name)

#else /* ! ENABLE_LOGGING */

#define log_debug(fmt, ...)
#define log_info(fmt, ...)
#define log_warn(fmt, ...)
#define log_err(fmt, ...)
#define log_msg(fmt, ...)

#define xlog_debug(logger, fmt, ...)
#define xlog_info(logger, fmt, ...)
#define xlog_warn(logger, fmt, ...)
#define xlog_err(logger, fmt, ...)
#define xlog_msg(logger, fmt, ...)

#define log_config(logger, lvl, prefix)

/* avoid warning for unused variables when disabling logging */
#define logger_init(name)
#define logger_free(name)

#endif /* ! ENABLE_LOGGING*/

#endif /* LOG_H */
