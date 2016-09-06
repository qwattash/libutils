
#include <stdarg.h>
#include <stdio.h>
#include <assert.h>

#include "log.h"

/**
 * Configure logging parameters
 */
void
_log_config(struct logger_handle *logger, enum log_level lvl,
	    const char *prefix)
{
  assert(logger != NULL);
  logger->level = lvl;
  logger->prefix = prefix;
}

/* logging functions */

void
_log_stdout(struct logger_handle *logger, enum log_level lvl,
	    const char *fmt, ...)
{
  va_list va;

  if (logger == NULL || logger->level <= lvl) {
    va_start(va, fmt);
    if (logger != NULL && logger->prefix != NULL)
      fprintf(stdout, logger->prefix);
    vfprintf(stdout, fmt, va);
    va_end(va);
  }
}

void
_log_stderr(struct logger_handle *logger, enum log_level lvl,
	    const char *fmt, ...)
{
  va_list va;

  if (logger == NULL || logger->level <= lvl) {
    va_start(va, fmt);
    if (logger != NULL && logger->prefix != NULL)
      fprintf(stderr, logger->prefix);
    vfprintf(stderr, fmt, va);
    va_end(va);
  }
}

inline struct logger_handle *
_logger_init(void)
{
  struct logger_handle *logger;

  logger = malloc(sizeof(struct logger_handle));
  assert(logger != NULL);

  return logger;
}
