#ifndef LOG_H
#define LOG_H

#include <stdio.h>
#include <stdarg.h>

#ifdef ENABLE_OUTPUT

static inline void
_log_info(const char *fmt, ...)
{
  va_list va;
  va_start(va, fmt);
  vfprintf(stdout, fmt, va);
  va_end(va);
}

static inline void
_log_err(const char *fmt, ...)
{
  va_list va;
  va_start(va, fmt);
  vfprintf(stderr, fmt, va);
  va_end(va);
}

#define log_info(fmt, ...) _log_info(fmt, ##__VA_ARGS__)
#define log_err(fmt, ...) _log_err(fmt, ##__VA_ARGS__)
#if DEBUG
#define log_debug(fmt, ...) _log_err(fmt, ##__VA_ARGS__)
#else
#define log_debug(fmt, ...)
#endif /* DEBUG */

#else /* ! ENABLE_OUTPUT */

#define log_info(fmt, ...)
#define log_err(fmt, ...)
#define log_debug(fmt, ...)
#endif /* ! ENABLE_OUTPUT */

#endif
