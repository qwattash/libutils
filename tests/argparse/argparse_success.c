
#include "argparse_test.h"

static void
check_posargs(argparse_t ap)
{
  char strarg[64];
  int intarg, err;
  
  err = argparse_posarg_get(ap, 0, strarg, 64);
  assert_int_equal(err, 0);
  assert_string_equal(strarg, "posarg");
  err = argparse_posarg_get(ap, 1, &intarg, 0);
  assert_int_equal(err, 0);
  assert_int_equal(intarg, 100);
}

static void
test_ap_posarg_ordering(void **state)
{
  argparse_t ap;
  int intarg, err;

  char *argv[] = {"./a.out", "0", "1", "2"};
  err = argparse_init(&ap, "Test parser", NULL);
  assert_int_equal(err, 0);

  err = argparse_posarg_add(ap, "pos0", T_INT, "");
  assert_int_equal(err, 0);
  err = argparse_posarg_add(ap, "pos1", T_INT, "");
  assert_int_equal(err, 0);
  err = argparse_posarg_add(ap, "pos2", T_INT, "");
  assert_int_equal(err, 0);

  err = argparse_parse(ap, 4, argv);
  assert_int_equal(err, 0);

  err = argparse_posarg_get(ap, 0, &intarg, 0);
  assert_int_equal(err, 0);
  assert_int_equal(intarg, 0);
  err = argparse_posarg_get(ap, 1, &intarg, 0);
  assert_int_equal(err, 0);
  assert_int_equal(intarg, 1);
  err = argparse_posarg_get(ap, 2, &intarg, 0);
  assert_int_equal(err, 0);
  assert_int_equal(intarg, 2);

  argparse_destroy(ap);
}

static void
test_ap_parse_success_all(void **state)
{
  int argc = 8;
  char *argv[] = {
    "./out",
    "-a",
    "foo",
    "-b",
    "10",
    "-c",
    "posarg",
    "100",
  };
  int err;
  char strarg[64];
  int intarg = 0;
  bool flag = false;

  err = argparse_parse(*state, argc, argv);
  assert_int_equal(err, 0);
  err = argparse_arg_get(*state, "arg1", strarg, 64);
  assert_int_equal(err, 0);
  err = argparse_arg_get(*state, "arg3", &flag, 0);
  assert_int_equal(err, 0);
  err = argparse_arg_get(*state, "arg2", &intarg, 0);
  assert_int_equal(err, 0);

  assert_string_equal(strarg, "foo");
  assert_int_equal(intarg, 10);
  assert_int_equal(flag, true);
  check_posargs(*state);
  
}

static void
test_ap_parse_success_required(void **state)
{
  int argc = 5;
  char *argv[] = {
    "./out",
    "-r",
    "required",
    "posarg",
    "100",
  };
  int err;
  char strarg[64];
  int intarg = 10;
  bool flag = false;

  memset(strarg, 0x00, 64);

  err = argparse_arg_add(*state, "req", 'r', T_STRING, "", true);
  assert_int_equal(err, 0);

  err = argparse_parse(*state, argc, argv);
  assert_int_equal(err, 0);
  err = argparse_arg_get(*state, "arg1", strarg, 64);
  assert_int_equal(err, E_NOARG);
  assert_string_equal(strarg, "");
  
  err = argparse_arg_get(*state, "arg3", &flag, 0);
  assert_int_equal(err, 0); /* flags are false but argument is not missing */
  assert_int_equal(flag, false);
  
  err = argparse_arg_get(*state, "arg2", &intarg, 0);
  assert_int_equal(err, E_NOARG);
  assert_int_equal(intarg, 10);
  
  err = argparse_arg_get(*state, "req", strarg, 64);
  assert_int_equal(err, 0);
  assert_string_equal(strarg, "required");
  
  check_posargs(*state);
}

static void
test_ap_parse_flag_unset(void **state)
{
  int argc = 3;
  char *argv[] = {
    "./out",
    "posarg",
    "100",
  };
  int err;
  bool flag = true;

  err = argparse_parse(*state, argc, argv);
  assert_int_equal(err, 0);

  err = argparse_arg_get(*state, "arg3", &flag, 0);
  assert_int_equal(err, 0);
  assert_int_equal(flag, false);
  check_posargs(*state);
}

static void
test_ap_parse_blanks(void **state)
{
  int argc = 8;
  char *argv[] = {
    "./out",
    "-a",
    " foo  ",
    "-b ",
    " 10  ",
    "-c",
    "  posarg",
    "100",
  };
  int err;
  char strarg[64];
  int intarg = 0;
  bool flag = false;

  err = argparse_parse(*state, argc, argv);
  assert_int_equal(err, 0);
  err = argparse_arg_get(*state, "arg1", strarg, 64);
  assert_int_equal(err, 0);
  err = argparse_arg_get(*state, "arg3", &flag, 0);
  assert_int_equal(err, 0);
  err = argparse_arg_get(*state, "arg2", &intarg, 0);
  assert_int_equal(err, 0);

  assert_string_equal(strarg, " foo  ");
  assert_int_equal(intarg, 10);
  assert_int_equal(flag, true);
  
  err = argparse_posarg_get(*state, 0, strarg, 64);
  assert_int_equal(err, 0);
  assert_string_equal(strarg, "  posarg");
  err = argparse_posarg_get(*state, 1, &intarg, 0);
  assert_int_equal(err, 0);
  assert_int_equal(intarg, 100);
}

int
main(int argc, char *argv[])
{
  const struct CMUnitTest tests[] = {
    /* argparser tests */
    cmocka_unit_test_setup_teardown(test_ap_parse_success_all,
    				    ap_setup,
    				    ap_teardown),
    cmocka_unit_test_setup_teardown(test_ap_parse_success_required,
    				    ap_setup,
				    ap_teardown),
    cmocka_unit_test_setup_teardown(test_ap_parse_flag_unset,
    				    ap_setup,
				    ap_teardown),
    cmocka_unit_test_setup_teardown(test_ap_parse_blanks,
    				    ap_setup,
				    ap_teardown),
    cmocka_unit_test(test_ap_posarg_ordering),
  };
  return cmocka_run_group_tests(tests, NULL, NULL);
}
