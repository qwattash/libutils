
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stddef.h>
#include <setjmp.h>
#include <assert.h>

#include <cmocka.h>

#include "utils_config.h"
#include "log.h"

/* stdio wrappers */

struct expect {
  const char *prefix;
  const char *message;
  FILE *fd;
};

/* mock vfprintf */
int
__wrap_vfprintf(FILE *fd, const char *fmt, va_list va)
{
  struct expect *result;
  char buffer[1024]; /* enough for tests */
  result = mock_ptr_type(struct expect *);
  
  assert_ptr_equal(fd, result->fd);
  assert_string_equal(fmt, result->message);

  return vsprintf(buffer, fmt, va);
}

/**
 * Test the hierarchical logging behaviour
 */

static void
test_tree_simple(void **state) {

  struct expect e;

  log_handle(toplevel);
  log_handle(child);

  log_init(&toplevel, NULL);
  log_init(&child, &toplevel);

  log_option_set(&toplevel, LOG_OPT_BACKEND, LOG_OPT_BACKEND_STDIO);
  log_option_set(&toplevel, LOG_OPT_LEVEL, LOG_OPT_LEVEL_WARNING);
  log_option_set(&toplevel, LOG_OPT_PREFIX, "toplevel");
  log_option_set(&toplevel, LOG_OPT_PREFIX_FMT, "%s->%s");
  log_option_set(&toplevel, LOG_OPT_MSG_FMT, "%s: %s");
  log_option_set(&child, LOG_OPT_BACKEND, LOG_OPT_BACKEND_BUBBLE);
  log_option_set(&child, LOG_OPT_LEVEL, LOG_OPT_LEVEL_NONE);
  log_option_set(&child, LOG_OPT_PREFIX, "child");

  e.prefix = "toplevel->child: ";
  
  /*
   * child: filter + bubble
   * toplevel: filter
   */
  xlog_debug(&child, "dbg_message");

  /*
   * child: filter + bubble
   * toplevel: handle
   */
  e.message = "toplevel->child: warn_message";
  e.fd = stdout;
  will_return(__wrap_vfprintf, &e);
  xlog_warn(&child, "warn_message");

  /*
   * child: filter + bubble
   * toplevel: handle
   */
  e.message = "toplevel->child: err_message";
  e.fd = stderr;
  will_return(__wrap_vfprintf, &e);
  xlog_err(&child, "err_message");
}

static void
test_tree_mixed_logging(void **state) {

  struct expect e_child;
  struct expect e_top;

  log_handle(toplevel);
  log_handle(child);

  log_init(&toplevel, NULL);
  log_init(&child, &toplevel);

  log_option_set(&toplevel, LOG_OPT_BACKEND, LOG_OPT_BACKEND_STDIO);
  log_option_set(&toplevel, LOG_OPT_LEVEL, LOG_OPT_LEVEL_WARNING);
  log_option_set(&toplevel, LOG_OPT_PREFIX, "toplevel");
  log_option_set(&toplevel, LOG_OPT_PREFIX_FMT, "%s->%s");
  log_option_set(&toplevel, LOG_OPT_MSG_FMT, "%s: %s");
  log_option_set(&child, LOG_OPT_BACKEND, LOG_OPT_BACKEND_STDIO);
  log_option_set(&child, LOG_OPT_LEVEL, LOG_OPT_LEVEL_DEBUG);
  log_option_set(&child, LOG_OPT_PREFIX, "child");
  log_option_set(&child, LOG_OPT_MSG_FMT, "%s: %s");

  e_child.prefix = "child";
  e_child.fd = stdout;
  e_top.prefix = "toplevel->child";
  e_top.fd = stdout;
  
  /*
   * child: handle + bubble
   * toplevel: filter
   */
  e_child.message = "child: dbg_message";
  will_return(__wrap_vfprintf, &e_child);
  xlog_debug(&child, "dbg_message");

  /*
   * child: handle + bubble
   * toplevel: handle
   */
  e_child.message = "child: warn_message";
  will_return(__wrap_vfprintf, &e_child);
  e_top.message = "toplevel->child: warn_message";
  will_return(__wrap_vfprintf, &e_top);
  xlog_warn(&child, "warn_message");

  /*
   * child: handle + bubble
   * toplevel: handle
   */
  e_child.message = "child: err_message";
  e_child.fd = stderr;
  will_return(__wrap_vfprintf, &e_child);
  e_top.message = "toplevel->child: err_message";
  e_top.fd = stderr;
  will_return(__wrap_vfprintf, &e_top);
  xlog_err(&child, "err_message");
}

int
main(int argc, char *argv[])
{
  int retval;
  
  const struct CMUnitTest test_default[] = {
    cmocka_unit_test(test_tree_simple),
    cmocka_unit_test(test_tree_mixed_logging)
  };

  retval = cmocka_run_group_tests_name("tree logger",
				       test_default,
				       NULL, NULL);
  return retval;
}
