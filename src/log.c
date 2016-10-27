
#include <stdarg.h>
#include <string.h>

#include "log.h"


/* logging functions */

void
_log(struct logger_handle *logger, int lvl,
	    const char *fmt, ...)
{
  va_list va;
  enum log_backend backend;
  FILE *fd;
#ifdef LINUX
  char *syslog_msg;
#endif

  if (logger != NULL && logger->level < lvl)
    return;

  if (logger == NULL)
    backend = LOG_BACKEND_STDIO;
  else
    backend = logger->backend;
  
  va_start(va, fmt);
  switch (backend) {
  case LOG_BACKEND_STDIO:
    if (lvl == LOG_ERR)
      fd = stderr;
    else
      fd = stdout;
    if (logger != NULL && logger->prefix != NULL)
      fprintf(fd, logger->prefix);
    vfprintf(fd, fmt, va);
    break;
  case LOG_BACKEND_FILE:
    if (logger != NULL) {
      if (logger->private.log_fd == NULL) {
	if (logger->log_file_path == NULL)
	  break;
	logger->private.log_fd = fopen(logger->log_file_path, "w");
	if (logger->private.log_fd == NULL)
	  break;
      }
      if (logger->prefix != NULL)
	fprintf(logger->private.log_fd, logger->prefix);
      vfprintf(logger->private.log_fd, fmt, va);
    }
    break;
#ifdef LINUX
  case LOG_BACKEND_SYSLOG:
    if (logger != NULL) {
      if (logger->private.syslog_open == false) {
	openlog(NULL, 0, LOG_DAEMON);
	logger->private.syslog_open = true;
      }
      if (logger->prefix != NULL) {
	syslog_msg = malloc(strlen(logger->prefix) + strlen(fmt) + 1);
	if (syslog_msg == NULL)
	  break;
	strcpy(syslog_msg, logger->prefix);
	strcat(syslog_msg, fmt);
	vsyslog(lvl, syslog_msg, va);
	free(syslog_msg);
      }
      else
	vsyslog(lvl, fmt, va);
    }
    break;
#endif
  default:
    break;
  }
  va_end(va);
}
