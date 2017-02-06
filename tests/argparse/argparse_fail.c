
#include "argparse_test.h"

static void
test_ap_parse_fail_required(void **state)
{
  int argc = 5;
  char *argv[] = {
    "./out",
    "-a",
    "foo",
    "posarg",
    "100",
  };
  int err;

  err = argparse_arg_add(*state, "req", 'r', T_STRING, "", true);
  assert_int_equal(err, ARGPARSE_OK);

  err = argparse_parse(*state, argc, argv);
  assert_int_equal(err, ARGPARSE_ERROR);
}

static void
test_ap_parse_fail_extra_posarg(void **state)
{
  int argc = 6;
  char *argv[] = {
    "./out",
    "-a",
    "foo",
    "posarg",
    "100",
    "extra",
  };
  int err;

  err = argparse_parse(*state, argc, argv);
  assert_int_equal(err, ARGPARSE_ERROR);
}

static void
test_ap_parse_fail_posarg_type(void **state)
{
  int argc = 5;
  char *argv[] = {
    "./out",
    "-a",
    "foo",
    "posarg",
    "not_a_number",
  };
  int err;
  
  err = argparse_parse(*state, argc, argv);
  assert_int_equal(err, ARGPARSE_ERROR);
}

static void
test_ap_parse_empty(void **state)
{
  int argc = 1;
  char *argv[] = {
    "./out",
  };
  int err;

  err = argparse_parse(*state, argc, argv);
  assert_int_equal(err, ARGPARSE_ERROR);
}

static void
test_ap_parse_empty_string_param(void **state)
{
  int argc = 2;
  char *argv[] = {
    "./out",
    "-s",
    "XXXXXXXXX" // guard this is a constant value that prevents random results
  };
  int err;

  err = argparse_arg_add(*state, "string", 's', T_STRING, "", false);
  assert_int_equal(err, ARGPARSE_OK);

  err = argparse_parse(*state, argc, argv);
  assert_int_equal(err, ARGPARSE_ERROR);
}

int
main(int argc, char *argv[])
{
  const struct CMUnitTest tests[] = {
    /* argparser tests */
    cmocka_unit_test_setup_teardown(test_ap_parse_fail_required,
    				    ap_setup,
				    ap_teardown),
    cmocka_unit_test_setup_teardown(test_ap_parse_fail_extra_posarg,
    				    ap_setup,
				    ap_teardown),
    cmocka_unit_test_setup_teardown(test_ap_parse_fail_posarg_type,
    				    ap_setup,
				    ap_teardown),
    cmocka_unit_test_setup_teardown(test_ap_parse_empty,
    				    ap_setup,
				    ap_teardown),
    cmocka_unit_test_setup_teardown(test_ap_parse_empty_string_param,
				    ap_setup_empty,
				    ap_teardown),
  };
  return cmocka_run_group_tests(tests, NULL, NULL);
}
