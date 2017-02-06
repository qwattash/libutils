
#include "argparse_test.h"

/*
 * Test basic subcommand initialization
 */
static void
test_subcommand_init(void **state)
{
  argparse_t sub;

  sub = argparse_subcmd_add(*state, "mycmd", "do something", NULL, NULL);
  assert_non_null(sub);
}

/*
 * Check that we can create nested subcommands
 */
static void
test_subcommand_nesting(void **state)
{
  argparse_t sub1, sub2, sub3, sub4;

  sub1 = argparse_subcmd_add(*state, "sub1", "top->sub1", NULL, NULL);
  assert_non_null(sub1);
  sub2 = argparse_subcmd_add(sub1, "sub2", "top->sub1->sub2", NULL, NULL);
  assert_non_null(sub2);
  sub3 = argparse_subcmd_add(sub1, "sub3", "top->sub1->sub3", NULL, NULL);
  assert_non_null(sub3);
  sub4 = argparse_subcmd_add(sub2, "sub4", "top->sub1->sub2->sub4", NULL, NULL);
  assert_non_null(sub4);
}

/*
 * Check that we can add options to a subcommand
 */
static void
test_subcommand_options(void **state)
{
  int err;
  argparse_t sub, nested;
  
  sub = argparse_subcmd_add(*state, "my_subcmd", "help msg", NULL, NULL);
  assert_non_null(sub);
  /* 
   * argument names voluntarily clash with the arguments names in the
   * initialized *state, they should be independant
   */
  err = argparse_arg_add(sub, "arg1", 'a', T_STRING, "", false);
  assert_int_equal(err, ARGPARSE_OK);
  err = argparse_arg_add(sub, "arg2", 'b', T_INT, "", false);
  assert_int_equal(err, ARGPARSE_OK);
  err = argparse_arg_add(sub, "arg3", 'c', T_FLAG, "", false);
  assert_int_equal(err, ARGPARSE_OK);

  err = argparse_posarg_add(sub, "pos1", T_STRING, "");
  assert_int_equal(err, ARGPARSE_OK);
  err = argparse_posarg_add(sub, "pos2", T_INT, "");
  assert_int_equal(err, ARGPARSE_OK);
  
  nested = argparse_subcmd_add(sub, "my_nested", "help msg", NULL, NULL);
  assert_non_null(nested);

  err = argparse_arg_add(nested, "arg1", 'a', T_STRING, "", false);
  assert_int_equal(err, ARGPARSE_OK);
  err = argparse_arg_add(nested, "arg2", 'b', T_INT, "", false);
  assert_int_equal(err, ARGPARSE_OK);
  err = argparse_arg_add(nested, "arg3", 'c', T_FLAG, "", false);
  assert_int_equal(err, ARGPARSE_OK);

  err = argparse_posarg_add(nested, "pos1", T_STRING, "");
  assert_int_equal(err, ARGPARSE_OK);
  err = argparse_posarg_add(nested, "pos2", T_INT, "");
  assert_int_equal(err, ARGPARSE_OK);
  
}

int
main(int argc, char *argv[])
{
  const struct CMUnitTest tests[] = {
    /* argparser tests */
    cmocka_unit_test_setup_teardown(test_subcommand_init,
    				    ap_setup,
				    ap_teardown),
    cmocka_unit_test_setup_teardown(test_subcommand_nesting,
    				    ap_setup,
				    ap_teardown),
    cmocka_unit_test_setup_teardown(test_subcommand_options,
    				    ap_setup,
				    ap_teardown),
  };
  return cmocka_run_group_tests(tests, NULL, NULL);
}
