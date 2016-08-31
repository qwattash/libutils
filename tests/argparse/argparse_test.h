#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "argparse.h"


#ifndef ARGPARSE_TEST_H
#define ARGPARSE_TEST_H

#define TEST_MAIN(f)					\
  int main(int argc, char *argv[])			\
  {							\
    const struct CMUnitTest tests[] = {			\
      cmocka_unit_test(f, ap_setup, ap_teardown);	\
    };							\
    cmocka_run_group_tests(tests, NULL, NULL);		\
  }

#define TEST_MAIN_SETUP(f)				\
  int main(int argc, char *argv[])			\
  {							\
    const struct CMUnitTest tests[] = {			\
      cmocka_unit_test(f, ap_setup, ap_teardown);	\
    };							\
    cmocka_run_group_tests(tests, NULL, NULL);		\
  }
  

static int
ap_setup(void **state)
{

  int err;
  argparse_t ap;

  err = argparse_init(&ap, "Test parser", NULL);
  assert_int_equal(err, 0);
  err = argparse_arg_add(ap, "arg1", 'a', T_STRING, "", false);
  assert_int_equal(err, 0);
  err = argparse_arg_add(ap, "arg2", 'b', T_INT, "", false);
  assert_int_equal(err, 0);
  err = argparse_arg_add(ap, "arg3", 'c', T_FLAG, "", false);
  assert_int_equal(err, 0);

  err = argparse_posarg_add(ap, "pos1", T_STRING, "");
  assert_int_equal(err, 0);
  err = argparse_posarg_add(ap, "pos2", T_INT, "");
  assert_int_equal(err, 0);

  *state = ap;
  return 0;
}

static int
ap_teardown(void **state)
{
  return argparse_destroy(*state);
}

#endif