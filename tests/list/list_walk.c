
#include "list_test.h"

static void
test_list_getitem(void **state)
{
  char *item = NULL;

  /* check getitem from an intialized 4-element list */
  item = list_get(*state, 0);
  assert_non_null(item);
  assert_string_equal(item, "0");
  assert_int_equal(list_length(*state), 4);
  item = list_get(*state, 2);
  assert_non_null(item);
  assert_string_equal(item, "2");
  assert_int_equal(list_length(*state), 4);
  item = list_get(*state, 3);
  assert_non_null(item);
  assert_string_equal(item, "3");
  assert_int_equal(list_length(*state), 4);
  item = list_get(*state, 1);
  assert_non_null(item);
  assert_string_equal(item, "1");
  assert_int_equal(list_length(*state), 4);
  item = list_get(*state, 2);
  assert_non_null(item);
  assert_string_equal(item, "2");
  assert_int_equal(list_length(*state), 4);
  assert_int_equal(dtor_count, 0);
}

static void
test_list_walk(void **state)
{
  int err;
  
  err = list_walk(*state, list_walk_cbk, NULL);
  assert_int_equal(err, UTILS_OK);
  assert_int_equal(walk_count, 4);
}

static void
test_list_indexof(void **state)
{
  int err;

  err = list_indexof(*state, "4");
  assert_int_equal(err, -1);

  err = list_indexof(*state, "0");
  assert_int_equal(err, 0);

  err = list_indexof(*state, "1");
  assert_int_equal(err, 1);

  err = list_indexof(*state, "2");
  assert_int_equal(err, 2);
}

int
main(int argc, char *argv[])
{
  const struct CMUnitTest tests[] = {
    cmocka_unit_test_setup_teardown(test_list_getitem,
    				    setup_list_3,
    				    teardown_list),
    cmocka_unit_test_setup_teardown(test_list_walk,
    				    setup_list_3,
    				    teardown_list),
    cmocka_unit_test_setup_teardown(test_list_indexof,
    				    setup_list_3,
    				    teardown_list),
  };
  return cmocka_run_group_tests(tests, NULL, NULL);
}
