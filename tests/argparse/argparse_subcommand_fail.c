
#include "argparse_test.h"

static int subcommand_options_cbk_count = 0;

static int
subcommand_options_cbk(argparse_t ap)
{
  subcommand_options_cbk_count++;
  return 0;
}

static void
test_ap_parse_fail_required(void **state)
{
  argparse_t sub;
  
  int argc = 9;
  char *argv[] = {
    "./out",
    "-r",
    "foo",
    "my_subcmd",
    "-b",
    "10",
    "sub_posarg",
    "posarg",
    "100",
  };
  int err;

  sub = argparse_subcmd_add(*state, "my_subcmd", "help msg",
			    subcommand_options_cbk);
  assert_non_null(sub);
  err = argparse_arg_add(*state, "req", 'r', T_STRING, "", true);
  assert_int_equal(err, 0);

  err = argparse_arg_add(sub, "sub_req", 'r', T_STRING, "", true);
  assert_int_equal(err, 0);
  err = argparse_arg_add(sub, "arg2", 'b', T_INT, "", false);
  assert_int_equal(err, 0);

  err = argparse_posarg_add(sub, "pos1", T_STRING, "");
  assert_int_equal(err, 0);

  err = argparse_parse(*state, argc, argv);
  assert_int_equal(err, E_PARSE);

  /* check that it has NOT been called */
  assert_int_equal(subcommand_options_cbk_count, 0);
  subcommand_options_cbk_count = 0;

}

static void
test_ap_parse_fail_extra_posarg(void **state)
{
  argparse_t sub;
  
  int argc = 10;
  char *argv[] = {
    "./out",
    "-a",
    "foo",
    "my_subcmd",
    "-b",
    "10",
    "sub_posarg",
    "extra_sub_posarg",
    /* this must be an int to avoid triggering a type error 
     * when parsing the posarg #2 of the root subcommand.
     */
    "100",
    "1000",
  };
  int err;

  sub = argparse_subcmd_add(*state, "my_subcmd", "help msg",
			    subcommand_options_cbk);
  assert_non_null(sub);

  err = argparse_arg_add(sub, "arg2", 'b', T_INT, "", false);
  assert_int_equal(err, 0);

  err = argparse_posarg_add(sub, "pos1", T_STRING, "");
  assert_int_equal(err, 0);

  err = argparse_parse(*state, argc, argv);
  assert_int_equal(err, E_PARSE);

  /* check that it has been called since the extra posarg is
   * detected only at the end
   */
  assert_int_equal(subcommand_options_cbk_count, 1);
  subcommand_options_cbk_count = 0;
}

static void
test_ap_parse_fail_posarg_type(void **state)
{
  argparse_t sub;
  
  int argc = 10;
  char *argv[] = {
    "./out",
    "-a",
    "foo",
    "my_subcmd",
    "-b",
    "10",
    "sub_posarg",
    "not_an_int",
    "posarg",
    "1000",
  };
  int err;

  sub = argparse_subcmd_add(*state, "my_subcmd", "help msg",
			    subcommand_options_cbk);
  assert_non_null(sub);

  err = argparse_arg_add(sub, "arg2", 'b', T_INT, "", false);
  assert_int_equal(err, 0);

  err = argparse_posarg_add(sub, "pos1", T_STRING, "");
  assert_int_equal(err, 0);
  err = argparse_posarg_add(sub, "pos2", T_INT, "");
  assert_int_equal(err, 0);

  err = argparse_parse(*state, argc, argv);
  assert_int_equal(err, E_PARSE);

  /* check that it has not been called */
  assert_int_equal(subcommand_options_cbk_count, 0);
  subcommand_options_cbk_count = 0;
}

static void
test_ap_parse_empty(void **state)
{
  argparse_t sub;
  
  int argc = 1;
  char *argv[] = {
    "./out",
  };
  int err;

  sub = argparse_subcmd_add(*state, "my_subcmd", "help msg",
			    subcommand_options_cbk);
  assert_non_null(sub);

  err = argparse_arg_add(sub, "arg2", 'b', T_INT, "", false);
  assert_int_equal(err, 0);

  err = argparse_posarg_add(sub, "pos1", T_STRING, "");
  assert_int_equal(err, 0);

  err = argparse_parse(*state, argc, argv);
  assert_int_equal(err, E_PARSE);

  /* check that it has not been called */
  assert_int_equal(subcommand_options_cbk_count, 0);
  subcommand_options_cbk_count = 0;
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
  };
  return cmocka_run_group_tests(tests, NULL, NULL);
}
