
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <setjmp.h>
#include <assert.h>

#include <cmocka.h>

#include "log.h"

struct expect_print {
  FILE *fd;
  const char *message;
  int argument;
};

/* stuff initialized by xlog_setup */
struct xlog_state {
  logger_t logger;
  struct expect_print debug;
  struct expect_print info;
  struct expect_print warn;
  struct expect_print err;
  struct expect_print msg;
  struct expect_print prefix;
  struct expect_print err_prefix;
};

int
__wrap_fprintf(FILE *fd, const char *fmt, ...)
{
  struct expect_print *expect;

  expect = mock_ptr_type(struct expect_print *);

  assert_ptr_equal(fd, expect->fd);
  assert_string_equal(fmt, expect->message);
  return strlen(fmt);
}

int
__wrap_vfprintf(FILE *fd, const char *fmt, va_list va)
{
  struct expect_print *expect;
  char buffer[1024]; /* enough for tests */
  int arg;
  expect = mock_ptr_type(struct expect_print *);

  assert_ptr_equal(fd, expect->fd);
  assert_string_equal(fmt, expect->message);
  arg = va_arg(va, int);
  assert_int_equal(arg, expect->argument);

  return vsprintf(buffer, fmt, va);
}

static void
test_log(void **state)
{
  struct expect_print e;

  e.fd = stdout;
  e.message = "debug message %d";
  e.argument = 10;
  
  will_return(__wrap_vfprintf, &e);
  log_debug("debug message %d", 10);

  e.message = "info message %d";
  will_return(__wrap_vfprintf, &e);
  log_info("info message %d", 10);

  e.message = "warning message %d";
  will_return(__wrap_vfprintf, &e);
  log_warn("warning message %d", 10);

  e.message = "user message %d";
  will_return(__wrap_vfprintf, &e);
  log_msg("user message %d", 10);

  e.fd = stderr;
  e.message = "error message %d";
  will_return(__wrap_vfprintf, &e);
  log_err("error message %d", 10);
}

static void
check_xlog(logger_t hl, struct xlog_state *st)
{
  xlog_debug(hl, st->debug.message, st->debug.argument);
  xlog_info(hl, st->info.message, st->info.argument);
  xlog_warn(hl, st->warn.message, st->warn.argument);
  xlog_err(hl, st->err.message, st->err.argument);
  xlog_msg(hl, st->msg.message, st->msg.argument);
}

static void
test_xlog_none(void **state)
{
  struct xlog_state *st = *state;
  
  log_config(st->logger, LOG_NONE, NULL);
  check_xlog(st->logger, *state);

  log_config(st->logger, LOG_NONE, st->prefix.message);
  check_xlog(st->logger, *state);
}

static void
test_xlog_msg(void **state)
{
  struct xlog_state *st = *state;
  
  log_config(st->logger, LOG_MSG, NULL);
  will_return(__wrap_vfprintf, &st->msg);
  check_xlog(st->logger, *state);

  log_config(st->logger, LOG_MSG, st->prefix.message);
  will_return(__wrap_vfprintf, &st->msg);
  will_return(__wrap_fprintf, &st->prefix);
  check_xlog(st->logger, *state);
}

static void
test_xlog_err(void **state)
{
#define MOCK_SET				\
  will_return(__wrap_vfprintf, &st->err);	\
  will_return(__wrap_vfprintf, &st->msg)
  
  struct xlog_state *st = *state;
  
  log_config(st->logger, LOG_ERROR, NULL);
  MOCK_SET;
  check_xlog(st->logger, *state);

  log_config(st->logger, LOG_ERROR, st->prefix.message);
  MOCK_SET;
  will_return(__wrap_fprintf, &st->err_prefix);
  will_return(__wrap_fprintf, &st->prefix);
  check_xlog(st->logger, *state);

#undef MOCK_SET
}

static void
test_xlog_warn(void **state)
{
#define MOCK_SET				\
  will_return(__wrap_vfprintf, &st->warn);	\
  will_return(__wrap_vfprintf, &st->err);	\
  will_return(__wrap_vfprintf, &st->msg)
  
  struct xlog_state *st = *state;
  
  log_config(st->logger, LOG_WARNING, NULL);
  MOCK_SET;
  check_xlog(st->logger, *state);

  log_config(st->logger, LOG_WARNING, st->prefix.message);
  MOCK_SET;
  will_return(__wrap_fprintf, &st->prefix);
  will_return(__wrap_fprintf, &st->err_prefix);
  will_return(__wrap_fprintf, &st->prefix);
  check_xlog(st->logger, *state);

#undef MOCK_SET
}

static void
test_xlog_info(void **state)
{
#define MOCK_SET				\
  will_return(__wrap_vfprintf, &st->info);	\
  will_return(__wrap_vfprintf, &st->warn);	\
  will_return(__wrap_vfprintf, &st->err);	\
  will_return(__wrap_vfprintf, &st->msg)
  
  struct xlog_state *st = *state;
  
  log_config(st->logger, LOG_INFO, NULL);
  MOCK_SET;
  check_xlog(st->logger, *state);

  log_config(st->logger, LOG_INFO, st->prefix.message);
  MOCK_SET;
  will_return_count(__wrap_fprintf, &st->prefix, 2);
  will_return(__wrap_fprintf, &st->err_prefix);
  will_return(__wrap_fprintf, &st->prefix);
  check_xlog(st->logger, *state);

#undef MOCK_SET
}

static void
test_xlog_debug(void **state)
{
#define MOCK_SET				\
  will_return(__wrap_vfprintf, &st->debug);	\
  will_return(__wrap_vfprintf, &st->info);	\
  will_return(__wrap_vfprintf, &st->warn);	\
  will_return(__wrap_vfprintf, &st->err);	\
  will_return(__wrap_vfprintf, &st->msg)
  
  struct xlog_state *st = *state;
  
  log_config(st->logger, LOG_DEBUG, NULL);
  MOCK_SET;
  check_xlog(st->logger, *state);

  log_config(st->logger, LOG_DEBUG, st->prefix.message);
  MOCK_SET;
  will_return_count(__wrap_fprintf, &st->prefix, 3);
  will_return(__wrap_fprintf, &st->err_prefix);
  will_return(__wrap_fprintf, &st->prefix);
  check_xlog(st->logger, *state);

#undef MOCK_SET
}

static int
xlog_setup(void **state)
{
  struct xlog_state *st = malloc(sizeof(struct xlog_state));
  assert(st != NULL);

  LOGGER_HANDLE(logger); /* define logger */
  logger_init(logger); /* init logger */
  st->logger = logger;
  st->debug.fd = stdout;
  st->debug.message = "debug message %d";
  st->debug.argument = 10;
  st->info.fd = stdout;
  st->info.message = "info message %d";
  st->info.argument = 10;
  st->warn.fd = stdout;
  st->warn.message = "warning message %d";
  st->warn.argument = 10;
  st->err.fd = stderr;
  st->err.message = "error message %d";
  st->err.argument = 10;
  st->msg.fd = stdout;
  st->msg.message = "user message %d";
  st->msg.argument = 10;
  st->prefix.fd = stdout;
  st->prefix.message = "log_prefix";
  /* st->prefix.argument is unused */
  st->err_prefix.fd = stderr;
  st->err_prefix.message = "log_prefix";
  /* st->prefix.argument is unused */
  *state = st;
  return 0;
}

static int
xlog_teardown(void **state)
{
  struct xlog_state *st = *state;
  logger_free(st->logger);
  free(*state);
  return 0;
}

int
main(int argc, char *argv[])
{
  const struct CMUnitTest tests[] = {
    cmocka_unit_test(test_log),
    cmocka_unit_test_setup_teardown(test_xlog_none,
				    xlog_setup,
				    xlog_teardown),
    cmocka_unit_test_setup_teardown(test_xlog_msg,
				    xlog_setup,
				    xlog_teardown),
    cmocka_unit_test_setup_teardown(test_xlog_err,
				    xlog_setup,
				    xlog_teardown),
    cmocka_unit_test_setup_teardown(test_xlog_warn,
				    xlog_setup,
				    xlog_teardown),
    cmocka_unit_test_setup_teardown(test_xlog_info,
				    xlog_setup,
				    xlog_teardown),
    cmocka_unit_test_setup_teardown(test_xlog_debug,
				    xlog_setup,
				    xlog_teardown),
  };
  return cmocka_run_group_tests(tests, NULL, NULL);
}
