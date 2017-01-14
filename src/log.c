
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
#ifdef HAVE_SYSLOG_H
  char *syslog_msg;
#endif

  /* build prefix 
   * XXX we should use a cache/hashmap of prefixes or a faster way
   * of building them
   */
  if (logger != NULL) {
    if (prefix_chain == NULL)
      prefix = (char *)logger->prefix;
    else {
      prefix = alloca(strlen(prefix_chain) + strlen(logger->prefix) + 1);
      if (prefix != NULL) {
	strcpy(prefix, logger->prefix);
	strcat(prefix, prefix_chain);
      }	
    }      
  }
  
  if (logger != NULL && logger->level >= lvl) {

    if (logger == NULL)
      backend = LOG_BACKEND_STDIO;
    else
      backend = logger->backend;
  
    switch (backend) {
    case LOG_BACKEND_STDIO:
      if (lvl == LOG_ERR)
	fd = stderr;
      else
	fd = stdout;
      if (prefix != NULL)
	fprintf(fd, prefix);
      vfprintf(fd, fmt, va);
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
      if (prefix != NULL)
	fprintf(logger->log_fd, prefix);
      vfprintf(logger->log_fd, fmt, va);
      break;
#ifdef HAVE_SYSLOG_H
    case LOG_BACKEND_SYSLOG:
      if (prefix != NULL) {
	syslog_msg = malloc(strlen(prefix) + strlen(fmt) + 1);
	if (syslog_msg == NULL)
	  break;
	strcpy(syslog_msg, prefix);
	strcat(syslog_msg, fmt);
	vsyslog(lvl, syslog_msg, va);
	free(syslog_msg);
      }
      else
	vsyslog(lvl, fmt, va);
      break;
#else
    case LOG_BACKEND_SYSLOG:
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
