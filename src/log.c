
#include <assert.h>
#include <stdarg.h>
#include <string.h>

#include "utils_config.h"
#include "log.h"

/* common log options */
const int log_opt_lvl_debug = LOG_DEBUG;
const int log_opt_lvl_info = LOG_INFO;
const int log_opt_lvl_warning = LOG_WARNING;
const int log_opt_lvl_err = LOG_ERR;
const int log_opt_lvl_alert = LOG_ALERT;
const int log_opt_lvl_none = LOG_NONE;
const enum log_backend log_opt_backend_stdio = LOG_BACKEND_STDIO;
const enum log_backend log_opt_backend_file = LOG_BACKEND_FILE;
const enum log_backend log_opt_backend_syslog = LOG_BACKEND_SYSLOG;
const enum log_backend log_opt_backend_bubble = LOG_BACKEND_BUBBLE;

static void _vlog(struct logger_handle *logger, int lvl,
		  const char *prefix_chain, const char *fmt, va_list va);

void
_log_init(struct logger_handle *logger, struct logger_handle *parent)
{
  /* init the logger to default value */
  assert(logger != NULL);

  logger->parent = parent;
  logger->level = LOG_ERR;
  if (parent == NULL)
    logger->backend = LOG_BACKEND_STDIO;
  else
    logger->backend = LOG_BACKEND_BUBBLE;
  logger->prefix = NULL;
  logger->log_file_path = NULL;
  logger->log_fd = NULL;
  logger->msg_fmt = "[%s] %s";
  logger->prefix_chain_fmt = "%s:%s";
}

void
_log_option_set(struct logger_handle *logger, enum log_conf_option opt,
		const void *value)
{
  enum log_backend backend;

  switch (opt) {
  case LOG_OPT_BACKEND:
    backend = *(enum log_backend *)value;
    switch (backend) {
    case LOG_BACKEND_STDIO:
    case LOG_BACKEND_FILE:
    case LOG_BACKEND_SYSLOG:
    case LOG_BACKEND_BUBBLE:
      logger->backend = backend;
      break;
    default:
      log_err("Invalid backend %d\n", backend);
    }
    break;
  case LOG_OPT_PREFIX:
    logger->prefix = (const char *)value;
    break;
  case LOG_OPT_LEVEL:
    logger->level = *(int *)value;
    break;
  case LOG_OPT_FILE:
    logger->log_file_path = (const char *)value;
    break;
  case LOG_OPT_MSG_FMT:
    logger->msg_fmt = (const char *)value;
    break;
  case LOG_OPT_PREFIX_FMT:
    logger->prefix_chain_fmt = (const char *)value;
    break;
  default:
    log_err("Invalid log option %d\n", opt);
  }
}

void
_log(struct logger_handle *logger, int lvl, const char *fmt, ...)
{
  va_list va;
  va_start(va, fmt);
  _vlog(logger, lvl, NULL, fmt, va);
  va_end(va);
}

static void
_vlog(struct logger_handle *logger, int lvl, const char *prefix_chain,
	    const char *fmt, va_list va)
{
  enum log_backend backend;
  FILE *fd;
  char *prefix = NULL;
  char *msg;

  /* build prefix */
  if (logger != NULL) {
    if (prefix_chain == NULL)
      prefix = (char *)logger->prefix;
    else {
      prefix = alloca(strlen(prefix_chain) + strlen(logger->prefix) +
		      strlen(logger->prefix_chain_fmt) + 1);
      if (prefix != NULL)
	sprintf(prefix, logger->prefix_chain_fmt, logger->prefix,
		prefix_chain);
    }
  }

  /* prevent any undefined prefix */
  if (prefix == NULL)
    prefix = "";
  
  if (logger == NULL || logger->level >= lvl) {

    if (logger == NULL) {
      backend = LOG_BACKEND_STDIO;
      msg = (char *)fmt;
    }
    else {
      backend = logger->backend;
      printf("prefix: %s\n", prefix);
      msg = alloca(strlen(fmt) + strlen(prefix) +
		   strlen(logger->msg_fmt) + 1);
      sprintf(msg, logger->msg_fmt, prefix, fmt);
    }
  
    switch (backend) {
    case LOG_BACKEND_STDIO:
      if (lvl == LOG_ERR)
	fd = stderr;
      else
	fd = stdout;
      vfprintf(fd, msg, va);
      break;
    case LOG_BACKEND_FILE:
      if (logger->log_fd == NULL) {
	if (logger->log_file_path == NULL)
	  break;
	logger->log_fd = fopen(logger->log_file_path, "w");
	if (logger->log_fd == NULL)
	  break;
	if (setvbuf(logger->log_fd, NULL, _IONBF, BUFSIZ)) {
	  fclose(logger->log_fd);
	  logger->log_fd = NULL;
	  break;
	}
      }
      vfprintf(logger->log_fd, msg, va);
      break;
    case LOG_BACKEND_SYSLOG:
#ifdef HAVE_SYSLOG_H
      vsyslog(lvl, msg, va);
      break;
#endif
    case LOG_BACKEND_BUBBLE:
      /* just fall through */
      break;
    }
  }
  /* bubble the log request to the parent logger */
  if (logger != NULL && logger->parent != NULL)
    _vlog(logger->parent, lvl, prefix, fmt, va);
}
