/**
 * The logger is handled by a layered hierarchy of
 * loggers. Each logger have the same handle properties
 * and log messages are propagated up in the logger chain.
 * Log messages are always propagated and every logger in the chain
 * has the opportunity to handle the message.
 * The prefixes are chained toghether when a message is propagated
 * up in the logger chain.
 * All loggers specify a backend that defines the
 * behaviour when a log message is handled.
 *
 * The logger handles are opaque and should always be handled
 * with the log or xlog macros so that all the logging code
 * is stripped away when logging is disabled.
 */

#ifndef LOG_H
#define LOG_H

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#include "utils_config.h"

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
 * LOG_BACKEND_STDIO: log to stdout and stderr
 * LOG_BACKEND_FILE: log to file
 * LOG_BACKEND_SYSLOG: log using linux syslog
 * LOG_BACKEND_BUBBLE: just bubble to parent logger
 */
enum log_backend {
  LOG_BACKEND_STDIO,
  LOG_BACKEND_FILE,
  LOG_BACKEND_SYSLOG,
  LOG_BACKEND_BUBBLE,
};

/**
 * Log option types, see macro log_option_set
 * LOG_OPT_BACKEND: set the backend value
 * LOG_OPT_PREFIX: set the prefix value,
 * the prefix string is not deallocated, it is
 * treated as a constant string.
 * LOG_OPT_LEVEL: set the logger level
 * LOG_OPT_FILE: set the log file path
 * LOG_OPT_FMT: set the format string used to generate
 * the log message. The fmt string must accept 3 string
 * arguments (prefix, prefix_chain, message)
 */
enum log_conf_option {
  LOG_OPT_BACKEND,
  LOG_OPT_PREFIX,
  LOG_OPT_LEVEL,
  LOG_OPT_FILE,
  LOG_OPT_MSG_FMT,
  LOG_OPT_PREFIX_FMT,
};

/**
 * Logger handle
 *
 * NOTE: This structure is OPAQUE
 * This should never be handled manually,
 * instead use the log_handle, log_init, log_option_set
 * and other log_* and xlog_* macros
 */
struct logger_handle {
  /* parent logger */
  struct logger_handle *parent;
  /* log level used to filter messages */
  int level;
  /* backend to use for messages handled */
  enum log_backend backend;
  /* message prefix */
  const char *prefix;
  /* path of the logfile when the backend is FILE */
  const char *log_file_path;
  /* format string that renders a log message
   * needs two string (%s) parameters (prefix, message)
   */
  const char *msg_fmt;
  /* format string used to compose the prefix from
   * a bubbled prefix from a sub-logger
   */
  const char *prefix_chain_fmt;
  /* log file fd for the FILE backend */
  FILE *log_fd;
};


/* private logging functions - use macros */
typedef struct logger_handle logger_t;

/**
 * Generic log function
 * @param logger: the logger handle, if NULL
 * the output is logged to stdout/stderr with no
 * filtering
 * @param lvl: log level of the message
 * @param fmt: format string
 * @param vararg: format arguments
 */
void _log(struct logger_handle *logger, int lvl,
	  const char *fmt, ...);

/**
 * Log handle initializer, add the logger to the logger
 * hierarchy with the given parent.
 * @param logger: the handle to initialize
 * @param parent: the parent logger, can be NULL
 */
void _log_init(struct logger_handle *logger,
	       struct logger_handle *parent);

/**
 * Configure logger parameters
 * @param logger: the logger handle
 * @param opt: the option to configure
 * @param value: the value of the option or pointer to it
 */
void _log_option_set(struct logger_handle *logger,
		     enum log_conf_option opt, const void *value);

/* 
 * common log option values to avoid having to specify a variable 
 * all the times
 */

const int log_opt_lvl_debug;
const int log_opt_lvl_info;
const int log_opt_lvl_warning;
const int log_opt_lvl_err;
const int log_opt_lvl_alert;
const int log_opt_lvl_none;

#define LOG_OPT_LEVEL_DEBUG (const void *)&log_opt_lvl_debug
#define LOG_OPT_LEVEL_INFO (const void *)&log_opt_lvl_info
#define LOG_OPT_LEVEL_WARNING (const void *)&log_opt_lvl_warning
#define LOG_OPT_LEVEL_ERR (const void *)&log_opt_lvl_err
#define LOG_OPT_LEVEL_ALERT (const void *)&log_opt_lvl_alert
#define LOG_OPT_LEVEL_NONE (const void *)&log_opt_lvl_none

const enum log_backend log_opt_backend_stdio;
const enum log_backend log_opt_backend_file;
const enum log_backend log_opt_backend_syslog;
const enum log_backend log_opt_backend_bubble;

#define LOG_OPT_BACKEND_STDIO (const void *)&log_opt_backend_stdio
#define LOG_OPT_BACKEND_FILE (const void *)&log_opt_backend_file
#define LOG_OPT_BACKEND_SYSLOG (const void *)&log_opt_backend_syslog
#define LOG_OPT_BACKEND_BUBBLE (const void *)&log_opt_backend_bubble

/* public logging API */
#ifdef ENABLE_LOGGING

#define log_handle(name) logger_t name
#define log_init(hnd, parent) _log_init(hnd, parent)
#define log_option_set(hnd, opt, value) _log_option_set(hnd, opt, value)

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

#define log_handle(name) (void)0
#define log_init(hnd, parent) (void)0
#define log_option_set(hnd, opt, value) (void)0

#define log_debug(fmt, ...) (void)0
#define log_info(fmt, ...) (void)0
#define log_warn(fmt, ...) (void)0
#define log_err(fmt, ...) (void)0
#define log_msg(fmt, ...) (void)0

#define xlog_debug(logger, fmt, ...) (void)0
#define xlog_info(logger, fmt, ...) (void)0
#define xlog_warn(logger, fmt, ...) (void)0
#define xlog_err(logger, fmt, ...) (void)0
#define xlog_msg(logger, fmt, ...) (void)0

#endif /* ! ENABLE_LOGGING*/

#endif /* LOG_H */
