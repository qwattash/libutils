
#include "argparse_test.h"

static int subcommand_options_cbk_count = 0;

static int
subcommand_options_cbk(argparse_t ap)
{
  subcommand_options_cbk_count++;
  return 0;
}

enum check_args_opt {
  ALL,
  NO_FLAG,
  ONLY_REQUIRED
};

static void
check_args(argparse_t root, argparse_t sub, enum check_args_opt opt)
{
  int err;
  char strarg[64];
  int intarg;
  bool flag;
  
  /* global args */
  err = argparse_arg_get(root, "arg1", strarg, 64);
  if (opt == ONLY_REQUIRED)
    assert_int_equal(err, E_NOARG);
  else {
    assert_int_equal(err, 0);
    assert_string_equal(strarg, "foo");
  }
  err = argparse_arg_get(root, "arg2", &intarg, 0);
  if (opt == ONLY_REQUIRED)
    assert_int_equal(err, E_NOARG);
  else {
    assert_int_equal(err, 0);
    assert_int_equal(intarg, 10);
  }
  err = argparse_arg_get(root, "arg3", &flag, 0);
  assert_int_equal(err, 0);
  if (opt == ONLY_REQUIRED)
    assert_int_equal(flag, false);
  else
    assert_int_equal(flag, true);
  err = argparse_posarg_get(root, 0, &strarg, 64);
  assert_int_equal(err, 0);
  assert_string_equal(strarg, "pos1");
  err = argparse_posarg_get(root, 1, &intarg, 0);
  assert_int_equal(err, 0);
  assert_int_equal(intarg, 10);
  /* subcommand args */
  err = argparse_arg_get(sub, "arg1", strarg, 64);
  assert_int_equal(err, 0);
  assert_string_equal(strarg, "sub_foo");
  err = argparse_arg_get(sub, "arg2", &intarg, 0);
  assert_int_equal(err, 0);
  assert_int_equal(intarg, 100);
  err = argparse_arg_get(sub, "arg3", &flag, 0);
  assert_int_equal(err, 0);
  if (opt == NO_FLAG || opt == ONLY_REQUIRED)
    assert_int_equal(flag, false);
  else
    assert_int_equal(flag, true);
  err = argparse_posarg_get(sub, 0, &strarg, 64);
  assert_int_equal(err, 0);
  assert_string_equal(strarg, "sub_pos1");
  err = argparse_posarg_get(sub, 1, &intarg, 0);
  assert_int_equal(err, 0);
  assert_int_equal(intarg, 100);
}

/*
 * Check that subcommands parse options correctly
 * simple case with only one subcommand
 */
static void
test_subcommand_options_parse(void **state)
{
  int err;
  argparse_t sub;
  int argc = 16;
  /* ./out -a foo -b 10 -c are associated with the top-level parser
   * my_subcmd -a sub_foo -b 100 -c are associated to my_subcmd
   * sub_pos1 100 are positional arguments associated to my_subcmd 
   * pos1 10 are positional arguments associated with the top-level perser
   */
  char *argv[] = {
    "./out",
    "-a",
    "foo",
    "-b",
    "10",
    "-c",
    "my_subcmd",
    "-a",
    "sub_foo",
    "-b",
    "100",
    "-c",
    "sub_pos1",
    "100",
    "pos1",
    "10"
  };
  /* check that the test is initialized properly */
  assert(subcommand_options_cbk_count == 0);
  
  sub = argparse_subcmd_add(*state, "my_subcmd", "help msg",
			    subcommand_options_cbk);
  assert_non_null(sub);
  err = argparse_arg_add(sub, "arg1", 'a', T_STRING, "", false);
  assert_int_equal(err, 0);
  err = argparse_arg_add(sub, "arg2", 'b', T_INT, "", false);
  assert_int_equal(err, 0);
  err = argparse_arg_add(sub, "arg3", 'c', T_FLAG, "", false);
  assert_int_equal(err, 0);

  err = argparse_posarg_add(sub, "pos1", T_STRING, "");
  assert_int_equal(err, 0);
  err = argparse_posarg_add(sub, "pos2", T_INT, "");
  assert_int_equal(err, 0);

  err = argparse_parse(*state, argc, argv);
  assert_int_equal(err, 0);
  
  assert_int_equal(subcommand_options_cbk_count, 1); /* check that it has been called */
  subcommand_options_cbk_count = 0;

  check_args(*state, sub, ALL);
}

/*
 * check that required options are accepted
 */
static void
test_subcommand_required(void **state)
{
  int err;
  argparse_t sub;

  int argc = 10;
  char *argv[] = {
    "./out",
    "my_subcmd",
    "-a",
    "sub_foo",
    "-b",
    "100",
    "sub_pos1",
    "100",
    "pos1",
    "10"
  };
  /* check that the test is initialized properly */
  assert(subcommand_options_cbk_count == 0);
  
  sub = argparse_subcmd_add(*state, "my_subcmd", "help msg",
			    subcommand_options_cbk);
  assert_non_null(sub);
  err = argparse_arg_add(sub, "arg1", 'a', T_STRING, "", true);
  assert_int_equal(err, 0);
  err = argparse_arg_add(sub, "arg2", 'b', T_INT, "", true);
  assert_int_equal(err, 0);
  err = argparse_arg_add(sub, "arg3", 'c', T_FLAG, "", false);
  assert_int_equal(err, 0);

  err = argparse_posarg_add(sub, "pos1", T_STRING, "");
  assert_int_equal(err, 0);
  err = argparse_posarg_add(sub, "pos2", T_INT, "");
  assert_int_equal(err, 0);

  err = argparse_parse(*state, argc, argv);
  assert_int_equal(err, 0);
  
  assert_int_equal(subcommand_options_cbk_count, 1); /* check that it has been called */
  subcommand_options_cbk_count = 0;
  
  check_args(*state, sub, ONLY_REQUIRED);
}

/*
 * check that unset flags are correctly parsed
 */
static void
test_subcommand_unset_flag(void **state)
{
  int err;
  argparse_t sub;

  int argc_flags = 15;
  char *argv_flags[] = {
    "./out",
    "-a",
    "foo",
    "-b",
    "10",
    "-c",
    "my_subcmd",
    "-a",
    "sub_foo",
    "-b",
    "100",
    "sub_pos1",
    "100",
    "pos1",
    "10"
  };
  
  /* check that the test is initialized properly */
  assert(subcommand_options_cbk_count == 0);
  
  sub = argparse_subcmd_add(*state, "my_subcmd", "help msg", subcommand_options_cbk);
  assert_non_null(sub);
  err = argparse_arg_add(sub, "arg1", 'a', T_STRING, "", false);
  assert_int_equal(err, 0);
  err = argparse_arg_add(sub, "arg2", 'b', T_INT, "", false);
  assert_int_equal(err, 0);
  err = argparse_arg_add(sub, "arg3", 'c', T_FLAG, "", false);
  assert_int_equal(err, 0);

  err = argparse_posarg_add(sub, "pos1", T_STRING, "");
  assert_int_equal(err, 0);
  err = argparse_posarg_add(sub, "pos2", T_INT, "");
  assert_int_equal(err, 0);

  err = argparse_parse(*state, argc_flags, argv_flags);
  assert_int_equal(err, 0);
  
  assert_int_equal(subcommand_options_cbk_count, 1); /* check that it has been called */
  subcommand_options_cbk_count = 0;
  check_args(*state, sub, NO_FLAG);
}

static void
test_posargs_only_in_subcmd(void **state)
{
  argparse_t ap, sub;
  int err;
  char foo[10];

  int argc = 3;
  char *argv[] = {"./a.out", "my_subcmd", "foo"};

  err = argparse_init(&ap, "Test parser", NULL);
  assert_int_equal(err, 0);
  sub = argparse_subcmd_add(ap, "my_subcmd", "Help msg", NULL);
  assert_non_null(sub);

  err = argparse_posarg_add(sub, "pos1", T_STRING, "");
  assert_int_equal(err, 0);

  err = argparse_parse(ap, argc, argv);
  assert_int_equal(err, 0);

  err = argparse_posarg_get(sub, 0, foo, 10);
  assert_int_equal(err, 0);
  assert_string_equal(foo, "foo");

  argparse_destroy(ap);
}

static void
test_ap_nested_parser_success(void **state)
{
  argparse_t ap, c1, c2;
  int err;
  char strarg[10];

  char *argv[] = {"./a.out", "cmd1", "cmd2", "-o", "option"};
  err = argparse_init(&ap, "Test parser", NULL);
  assert_int_equal(err, 0);

  c1 = argparse_subcmd_add(ap, "cmd1", "Command 1", NULL);
  c2 = argparse_subcmd_add(c1, "cmd2", "Command 2", NULL);

  err = argparse_arg_add(c2, "option", 'o', T_STRING, "Option", false);
  assert_int_equal(err, 0);
  
  err = argparse_parse(ap, 5, argv);
  assert_int_equal(err, 0);

  err = argparse_arg_get(c2, "option", strarg, 10);
  assert_int_equal(err, 0);
  assert_string_equal(strarg, "option");

  argparse_destroy(ap);
}

int
main(int argc, char *argv[])
{
  const struct CMUnitTest tests[] = {
    /* argparser tests */
    cmocka_unit_test_setup_teardown(test_subcommand_options_parse,
    				    ap_setup,
				    ap_teardown),
    cmocka_unit_test_setup_teardown(test_subcommand_required,
    				    ap_setup,
				    ap_teardown),
    cmocka_unit_test_setup_teardown(test_subcommand_unset_flag,
    				    ap_setup,
				    ap_teardown),
    cmocka_unit_test(test_ap_nested_parser_success),
    cmocka_unit_test(test_posargs_only_in_subcmd),
  };
  return cmocka_run_group_tests(tests, NULL, NULL);
}
