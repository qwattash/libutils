
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <setjmp.h>
#include <assert.h>

#include <cmocka.h>

#include "libutils/config.h"
#include "libutils/log.h"

struct expect {
  FILE *fd;
  const char *prefix;
  const char *message;
  int argument;

  /* file backend stuff */
  const char *log_file_path;
  const char *log_file_mode;
};

/* 
 * stuff initialized by xlog_setup with expected 
 * values of parameters to mocked functions
*/
struct xlog_state {
  struct logger_handle logger;
  struct expect result;
};
  
/*
 * mock FILE * returned by fopen
 */
FILE *mock_fd = (FILE*) 0x1000;

static void mock_setup(struct xlog_state *st, int lvl);

/*
 * mock vfprintf
 */
int
__wrap_vfprintf(FILE *fd, const char *fmt, va_list va)
{
  struct expect *result;
  char buffer[1024]; /* enough for tests */
  int arg;
  result = mock_ptr_type(struct expect *);
  
  assert_ptr_equal(fd, result->fd);
  assert_string_equal(fmt, result->message);
  arg = va_arg(va, int);
  assert_int_equal(arg, result->argument);

  return vsprintf(buffer, fmt, va);
}

/*
 * mock fopen for file logging
 */
FILE*
__wrap_fopen(const char *path, const char *mode)
{
  struct expect *result;

  result = mock_ptr_type(struct expect *);
  
  assert_string_equal(path, result->log_file_path);
  assert_string_equal(mode, result->log_file_mode);
  return mock_fd;
}


/*
 * mock setvbuf for file logging
 */
int
__wrap_setvbuf(FILE *fd, char *buffer, int mode, size_t size)
{
  struct expect *result;

  result = mock_ptr_type(struct expect *);

  assert_ptr_equal(fd, result->fd);
  assert_int_equal(mode, _IONBF);
  return 0;
}

#ifdef HAVE_SYSLOG_H
/*
 * mock syslog logging function
 */
void
__wrap_vsyslog(int lvl, const char *fmt, va_list va)
{
  struct expect *result;
  int arg;
  
  result = mock_ptr_type(struct expect *);

  assert_string_equal(fmt, result->message);
  arg = va_arg(va, int);
  assert_int_equal(arg, result->argument);
}
#endif

/*
 * test logging macros that do not specify a logger
 */
static void
test_log(void **state)
{
  struct expect e;

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

/*
 * Test LOG_HANDLE macro for static logger definition 
 */
static void
test_log_handle(void **state)
{
  log_handle(logger_struct);
  struct logger_handle *logger = &logger_struct;
  struct expect e;

  log_init(logger, NULL);
  log_option_set(logger, LOG_OPT_LEVEL, LOG_OPT_LEVEL_DEBUG);
  log_option_set(logger, LOG_OPT_PREFIX, "prefix");
  log_option_set(logger, LOG_OPT_MSG_FMT, "%s %s");

  e.fd = stdout;
  e.message = "prefix debug message %d";
  e.prefix = "prefix";
  e.argument = 10;

  will_return(__wrap_vfprintf, &e);
  xlog_debug(logger, "debug message %d", 10);

  e.message = "prefix info message %d";
  will_return(__wrap_vfprintf, &e);
  xlog_info(logger, "info message %d", 10);

  e.message = "prefix warning message %d";
  will_return(__wrap_vfprintf, &e);
  xlog_warn(logger, "warning message %d", 10);

  e.message = "prefix user message %d";
  will_return(__wrap_vfprintf, &e);
  xlog_msg(logger, "user message %d", 10);

  e.fd = stderr;
  e.message = "prefix error message %d";
  will_return(__wrap_vfprintf, &e);
  xlog_err(logger, "error message %d", 10);
}

/*
 * Test function that performs the test as specified in the state
 */
static void
test_xlog_msg(void **state)
{
  struct xlog_state *st = *state;
  char message[1024];
  sprintf(message, "%s %s", st->result.prefix, "logging %d at level MSG");
  
  mock_setup(st, LOG_ALERT);
  st->result.message = message;
  st->result.argument = 1;
  xlog_msg(&st->logger, "logging %d at level MSG", 1);
}

static void
test_xlog_err(void **state)
{
  struct xlog_state *st = *state;
  char message[1024];
  sprintf(message, "%s %s", st->result.prefix, "logging %d at level ERR");

  mock_setup(st, LOG_ERR);
  st->result.message = message;
  st->result.argument = 10;
  xlog_err(&st->logger, "logging %d at level ERR", 10);
}

static void
test_xlog_warn(void **state)
{
  struct xlog_state *st = *state;
  char message[1024];
  sprintf(message, "%s %s", st->result.prefix, "logging %d at level WARN");
  
  mock_setup(st, LOG_WARNING);
  st->result.message = message;
  st->result.argument = 100;
  xlog_warn(&st->logger, "logging %d at level WARN", 100);
}

static void
test_xlog_info(void **state)
{
  struct xlog_state *st = *state;
  char message[1024];
  sprintf(message, "%s %s", st->result.prefix, "logging %d at level INFO");
  
  mock_setup(st, LOG_INFO);
  st->result.message = message;
  st->result.argument = 1000;
  xlog_info(&st->logger, "logging %d at level INFO", 1000);
}

static void
test_xlog_debug(void **state)
{
  struct xlog_state *st = *state;
  char message[1024];
  sprintf(message, "%s %s", st->result.prefix, "logging %d at level DEBUG");

  mock_setup(st, LOG_DEBUG);
  st->result.message = message;
  st->result.argument = 10000;
  xlog_debug(&st->logger, "logging %d at level DEBUG", 10000);
}


/*
 * Test setup functions.
 *
 * Each test setup initialises the logger and the expected result 
 * for a different combination of the logging backend and logger
 * prefix
 */

/**
 * Setup the expected mock calls
 * this must be called from the actual test function as
 * doing this in the setup functions seems to fail.
 */
static void mock_setup(struct xlog_state *st, int lvl)
{
  if (st->logger.level >= lvl) {
    if (st->logger.backend == LOG_BACKEND_FILE) {
      will_return(__wrap_fopen, &st->result);
      will_return(__wrap_setvbuf, &st->result);
    }
    if (st->logger.backend == LOG_BACKEND_SYSLOG) {
      will_return(__wrap_vsyslog, &st->result);
    }
    else
      will_return(__wrap_vfprintf, &st->result);
  }
}

/**
 * Setup for tests
 * Backend: STDIO
 * Prefix: NULL
 * Output: stdout
 */
static int
xlog_setup_stdio(void **state)
{
  struct xlog_state *st = *state;
  printf("Backend: STDIO\n");
  log_option_set(&st->logger, LOG_OPT_BACKEND, LOG_OPT_BACKEND_STDIO);
  log_option_set(&st->logger, LOG_OPT_PREFIX, NULL);
  st->result.fd = stdout;
  st->result.prefix = "";
  return 0;
}

/**
 * Setup for tests
 * Backend: STDIO
 * Prefix: NULL
 * Output: stderr
 */
static int
xlog_setup_stdio_err(void **state)
{
  struct xlog_state *st = *state;
  xlog_setup_stdio(state);
  st->result.fd = stderr;
  return 0;
}

/**
 * Setup for tests 
 * Backend: STDIO
 * Prefix: "stdio backend prefix"
 * Output: stdout
 */
static int
xlog_setup_stdio_prefix(void **state)
{
  struct xlog_state *st = *state;
  xlog_setup_stdio(state);
  log_option_set(&st->logger, LOG_OPT_PREFIX, "stdio prefix");
  st->result.prefix = "stdio prefix";
  return 0;
}

/**
 * Setup for tests 
 * Backend: STDIO
 * Prefix: "stdio backend prefix"
 * Output: stderr
 */
static int
xlog_setup_stdio_prefix_err(void **state)
{
  struct xlog_state *st = *state;
  xlog_setup_stdio_prefix(state);
  st->result.fd = stderr;
  return 0;
}

/**
 * Setup file backend
 * Backend: FILE
 * Prefix: NULL
 * Output: mock_fd
 */
static int
xlog_setup_file(void **state)
{
  struct xlog_state *st = *state;
  printf("Backend: FILE\n");
  log_option_set(&st->logger, LOG_OPT_BACKEND, LOG_OPT_BACKEND_FILE);
  log_option_set(&st->logger, LOG_OPT_PREFIX, NULL);
  log_option_set(&st->logger, LOG_OPT_FILE, "path/to/log/file.txt");
  st->logger.log_fd = NULL;
  st->result.log_file_path = "path/to/log/file.txt";
  st->result.fd = mock_fd;
  st->result.log_file_mode = "w";
  st->result.prefix = "";
  return 0;
}

/**
 * Setup file backend
 * Backend: FILE
 * Prefix: "file backend prefix"
 * Output: mock_fd
 */
static int
xlog_setup_file_prefix(void **state)
{
  struct xlog_state *st = *state;
  xlog_setup_file(state);
  log_option_set(&st->logger, LOG_OPT_PREFIX, "file backend prefix");
  st->result.prefix = "file backend prefix";
  return 0;
}

/**
 * Setup file backend
 * Backend: SYSLOG
 * Prefix: NULL
 */
static int
xlog_setup_syslog(void **state)
{
  struct xlog_state *st = *state;
  printf("Backend: SYSLOG\n");
  log_option_set(&st->logger, LOG_OPT_BACKEND, LOG_OPT_BACKEND_SYSLOG);
  log_option_set(&st->logger, LOG_OPT_PREFIX, NULL);
  st->result.log_file_path = NULL;
  st->result.fd = mock_fd;
  st->result.log_file_mode = NULL;
  st->result.prefix = "";
  return 0;
}

/**
 * Setup file backend
 * Backend: SYSLOG
 * Prefix: "syslog backend prefix"
 */
static int
xlog_setup_syslog_prefix(void **state)
{
  struct xlog_state *st = *state;
  xlog_setup_syslog(state);
  log_option_set(&st->logger, LOG_OPT_PREFIX, "syslog backend prefix");
  st->result.prefix = "syslog backend prefix";
  return 0;
}

/*
 * Test group setup functions.
 *
 * Each group initialises the logger and the expected result 
 * with a different log level
 */

static int
xlog_setup_group_none(void **state)
{
  struct xlog_state *st = malloc(sizeof(struct xlog_state));
  assert(st != NULL);
  log_init(&st->logger, NULL);
  log_option_set(&st->logger, LOG_OPT_LEVEL, LOG_OPT_LEVEL_NONE);
  log_option_set(&st->logger, LOG_OPT_MSG_FMT, "%s %s");
  *state = st;
  return 0;
}

static int
xlog_setup_group_msg(void **state)
{
  struct xlog_state *st = malloc(sizeof(struct xlog_state));
  assert(st != NULL);
  log_init(&st->logger, NULL);
  log_option_set(&st->logger, LOG_OPT_LEVEL, LOG_OPT_LEVEL_ALERT);
  log_option_set(&st->logger, LOG_OPT_MSG_FMT, "%s %s");
  *state = st;
  return 0;
}

static int
xlog_setup_group_err(void **state)
{
  struct xlog_state *st = malloc(sizeof(struct xlog_state));
  assert(st != NULL);
  log_init(&st->logger, NULL);
  log_option_set(&st->logger, LOG_OPT_LEVEL, LOG_OPT_LEVEL_ERR);
  log_option_set(&st->logger, LOG_OPT_MSG_FMT, "%s %s");
  *state = st;
  return 0;
}

static int
xlog_setup_group_warn(void **state)
{
  struct xlog_state *st = malloc(sizeof(struct xlog_state));
  assert(st != NULL);
  log_init(&st->logger, NULL);
  log_option_set(&st->logger, LOG_OPT_LEVEL, LOG_OPT_LEVEL_WARNING);
  log_option_set(&st->logger, LOG_OPT_MSG_FMT, "%s %s");
  *state = st;
  return 0;
}

static int
xlog_setup_group_info(void **state)
{
  struct xlog_state *st = malloc(sizeof(struct xlog_state));
  assert(st != NULL);
  log_init(&st->logger, NULL);
  log_option_set(&st->logger, LOG_OPT_LEVEL, LOG_OPT_LEVEL_INFO);
  log_option_set(&st->logger, LOG_OPT_MSG_FMT, "%s %s");
  *state = st;
  return 0;
}

static int
xlog_setup_group_debug(void **state)
{
  struct xlog_state *st = malloc(sizeof(struct xlog_state));
  assert(st != NULL);
  log_init(&st->logger, NULL);
  log_option_set(&st->logger, LOG_OPT_LEVEL, LOG_OPT_LEVEL_INFO);
  log_option_set(&st->logger, LOG_OPT_MSG_FMT, "%s %s");
  *state = st;
  return 0;
}

static int
xlog_teardown_group(void **state)
{
  free(*state);
  return 0;
}

int
main(int argc, char *argv[])
{
  int retval;
  
  const struct CMUnitTest test_default[] = {
    cmocka_unit_test(test_log),
    cmocka_unit_test(test_log_handle),
  };

  const struct CMUnitTest test_level_group[] = {
    cmocka_unit_test_setup(test_xlog_msg, xlog_setup_stdio),
    cmocka_unit_test_setup(test_xlog_err, xlog_setup_stdio_err),
    cmocka_unit_test_setup(test_xlog_warn, xlog_setup_stdio),
    cmocka_unit_test_setup(test_xlog_info, xlog_setup_stdio),
    cmocka_unit_test_setup(test_xlog_debug, xlog_setup_stdio),
    
    cmocka_unit_test_setup(test_xlog_msg, xlog_setup_stdio_prefix),
    cmocka_unit_test_setup(test_xlog_err, xlog_setup_stdio_prefix_err),
    cmocka_unit_test_setup(test_xlog_warn, xlog_setup_stdio_prefix),
    cmocka_unit_test_setup(test_xlog_info, xlog_setup_stdio_prefix),
    cmocka_unit_test_setup(test_xlog_debug, xlog_setup_stdio_prefix),

    cmocka_unit_test_setup(test_xlog_msg, xlog_setup_file),
    cmocka_unit_test_setup(test_xlog_err, xlog_setup_file),
    cmocka_unit_test_setup(test_xlog_warn, xlog_setup_file),
    cmocka_unit_test_setup(test_xlog_info, xlog_setup_file),
    cmocka_unit_test_setup(test_xlog_debug, xlog_setup_file),
    
    cmocka_unit_test_setup(test_xlog_msg, xlog_setup_file_prefix),
    cmocka_unit_test_setup(test_xlog_err, xlog_setup_file_prefix),
    cmocka_unit_test_setup(test_xlog_warn, xlog_setup_file_prefix),
    cmocka_unit_test_setup(test_xlog_info, xlog_setup_file_prefix),
    cmocka_unit_test_setup(test_xlog_debug, xlog_setup_file_prefix),

    cmocka_unit_test_setup(test_xlog_msg, xlog_setup_syslog),
    cmocka_unit_test_setup(test_xlog_err, xlog_setup_syslog),
    cmocka_unit_test_setup(test_xlog_warn, xlog_setup_syslog),
    cmocka_unit_test_setup(test_xlog_info, xlog_setup_syslog),
    cmocka_unit_test_setup(test_xlog_debug, xlog_setup_syslog),
    
    cmocka_unit_test_setup(test_xlog_msg, xlog_setup_syslog_prefix),
    cmocka_unit_test_setup(test_xlog_err, xlog_setup_syslog_prefix),
    cmocka_unit_test_setup(test_xlog_warn, xlog_setup_syslog_prefix),
    cmocka_unit_test_setup(test_xlog_info, xlog_setup_syslog_prefix),
    cmocka_unit_test_setup(test_xlog_debug, xlog_setup_syslog_prefix),
  };

  retval = cmocka_run_group_tests_name("default logger",
				       test_default,
				       NULL, NULL);
  retval += cmocka_run_group_tests_name("logger lvl NONE",
					test_level_group,
					xlog_setup_group_none,
					xlog_teardown_group);
  retval += cmocka_run_group_tests_name("logger lvl MSG",
					test_level_group,
					xlog_setup_group_msg,
					xlog_teardown_group);
  retval += cmocka_run_group_tests_name("logger lvl ERR",
					test_level_group,
					xlog_setup_group_err,
					xlog_teardown_group);
  retval += cmocka_run_group_tests_name("logger lvl WARNING",
					test_level_group,
					xlog_setup_group_warn,
					xlog_teardown_group);
  retval += cmocka_run_group_tests_name("logger lvl INFO",
					test_level_group,
					xlog_setup_group_info,
					xlog_teardown_group);
  retval += cmocka_run_group_tests_name("logger lvl DEBUG",
					test_level_group,
					xlog_setup_group_debug,
					xlog_teardown_group);
  return retval;
}
