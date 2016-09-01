
#include "argparse_test.h"

static void
test_ap_init_destroy(void **state)
{
  int err;
  argparse_t ap;

  err = argparse_init(&ap, "Test parser", NULL);
  assert_int_equal(err, 0);
  err = argparse_destroy(ap);
  assert_int_equal(err, 0);
}

static void
test_ap_full_destroy(void **state)
{
  int err;
  argparse_t ap;

  err = argparse_init(&ap, "Test parser", NULL);
  assert_int_equal(err, 0);
  err = argparse_arg_add(ap, "a", 'a', T_STRING, "help a", false);
  assert_int_equal(err, 0);
  err = argparse_arg_add(ap, "b", 'b', T_INT, "help b", false);
  assert_int_equal(err, 0);
  err = argparse_arg_add(ap, "c", 'c', T_FLAG, "help c", false);

  err = argparse_posarg_add(ap, "pa", T_STRING, "help a");
  assert_int_equal(err, 0);
  err = argparse_posarg_add(ap, "pb", T_INT, "help b");
  assert_int_equal(err, 0);
  err = argparse_destroy(ap);
  assert_int_equal(err, 0);
}

int
main(int argc, char *argv[])
{
  const struct CMUnitTest tests[] = {
    /* argparser tests */
    cmocka_unit_test(test_ap_init_destroy),
    cmocka_unit_test(test_ap_full_destroy),
  };
  return cmocka_run_group_tests(tests, NULL, NULL);
}
