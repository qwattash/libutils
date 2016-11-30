#ifndef LOG_H
#define LOG_H

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#include "config.h"

#ifdef HAVE_SYSLOG_H
/* Include the definition of log levels */
#include <syslog.h>

#define LOG_NONE -999
#else /* LINUX */
/**
 * Log levels
 */
#define  LOG_DEBUG 5
#define  LOG_INFO 4
#define  LOG_WARNING 3
#define  LOG_ERR 2
#define  LOG_ALERT 1
#define  LOG_NONE 0
#endif /* LINUX */

/**
 * Logging backend
 *
 * LOG_STDIO: log to stdout and stderr
 * LOG_FILE: log to file
 * LOG_SYSLOG: log using linux syslog
 */
enum log_backend {
  LOG_BACKEND_STDIO,
  LOG_BACKEND_FILE,
  LOG_BACKEND_SYSLOG,
};

/**
 * Logger handle
 */
struct logger_handle {
  int level;
  enum log_backend backend;
  const char *prefix;
  const char *log_file_path;
  union {
    // private fields
    FILE *log_fd;
    bool syslog_open;
  } private;
};


/* private logging functions - use macros */
void _log(struct logger_handle *logger, int lvl,
	  const char *fmt, ...);

#ifdef ENABLE_LOGGING

/* macro to create instance of logger handle */
#define LOG_HANDLE(name, level, prefix)		\
  struct logger_handle name = {			\
    level,					\
    LOG_BACKEND_STDIO,				\
    prefix,					\
    NULL,					\
    {.log_fd = NULL},				\
  }

#ifdef LOG_NODEBUG
#define log_debug(fmt, ...)
#define xlog_debug(logger, fmt, ...)
#else /* ! LOG_NODEBUG */
#define log_debug(fmt, ...) _log(NULL, LOG_DEBUG, fmt, ##__VA_ARGS__)
#define xlog_debug(logger, fmt, ...) _log(logger, LOG_DEBUG, fmt, ##__VA_ARGS__)
#endif /* ! LOG_NODEBUG */

#define log_info(fmt, ...) _log(NULL, LOG_INFO, fmt, ##__VA_ARGS__)
#define log_warn(fmt, ...) _log(NULL, LOG_WARNING, fmt, ##__VA_ARGS__)
#define log_err(fmt, ...) _log(NULL, LOG_ERR, fmt, ##__VA_ARGS__)
#define log_msg(fmt, ...) _log(NULL, LOG_ALERT, fmt, ##__VA_ARGS__)

#define xlog_info(logger, fmt, ...)			\
  _log(logger, LOG_INFO, fmt, ##__VA_ARGS__)
#define xlog_warn(logger, fmt, ...)			\
  _log(logger, LOG_WARNING, fmt, ##__VA_ARGS__)
#define xlog_err(logger, fmt, ...)			\
  _log(logger, LOG_ERR, fmt, ##__VA_ARGS__)
#define xlog_msg(logger, fmt, ...)			\
  _log(logger, LOG_ALERT, fmt, ##__VA_ARGS__)

#else /* ! ENABLE_LOGGING */

#define LOG_HANDLE(name, level, prefix)

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

#endif /* ! ENABLE_LOGGING*/

#endif /* LOG_H */
