
#include "list_test.h"

static void
test_list_push_pop(void **state)
{
  int err;
  char *item;

  /* push an item */
  err = list_push(*state, "item");
  assert_int_equal(err, 0);
  assert_int_equal(ctor_count, 1);
  assert_int_equal(dtor_count, 0);
  assert_int_equal(list_length(*state), 1);
  /* pop the item and check the state */
  item = list_pop(*state);
  assert_int_equal(ctor_count, 1);
  /* pop does not deallocate the item */
  assert_int_equal(dtor_count, 0);
  assert_non_null(item);
  assert_string_equal(item, "item");
  assert_int_equal(list_length(*state), 0);
}

static void
test_list_insert(void **state)
{
  int err;
  char *item;
  /* insert elements to position 0, 1, 3 
   * make sure that element 2 is created automatically
   */
  err = list_insert(*state, "0", 0);
  assert_int_equal(err, 0);
  assert_int_equal(list_length(*state), 1);
  err = list_insert(*state, "1", 1);
  assert_int_equal(err, 0);
  assert_int_equal(list_length(*state), 2);
  err = list_insert(*state, "3", 3);
  assert_int_equal(err, 0);
  assert_int_equal(list_length(*state), 4);
  /* the ctor is called also for the empty item list[2]*/
  assert_int_equal(ctor_count, 4);
  assert_int_equal(dtor_count, 0);

  /*
   * check that the elements are in the correct order
   */
  item = list_pop(*state);
  assert_string_equal(item, "0");
  item = list_pop(*state);
  assert_string_equal(item, "1");
  item = list_pop(*state);
  assert_ptr_equal(item, NULL);
  item = list_pop(*state);
  assert_string_equal(item, "3");
  assert_int_equal(ctor_count, 4);
  assert_int_equal(dtor_count, 0);
}

static void
test_list_append_empty(void **state)
{
  int err = 0;
  char *item;

  /* push an item */
  err = list_append(*state, "item");
  assert_int_equal(err, 0);
  assert_int_equal(ctor_count, 1);
  assert_int_equal(dtor_count, 0);
  assert_int_equal(list_length(*state), 1);
  item = list_getitem(*state, 0);
  assert_string_equal(item, "item");
}

static void
test_list_append_full(void **state)
{
  int err = 0;
  char *item;

  /* push an item */
  err = list_append(*state, "appended");
  assert_int_equal(err, 0);
  assert_int_equal(ctor_count, 5);
  assert_int_equal(dtor_count, 0);
  assert_int_equal(list_length(*state), 5);
  item = list_getitem(*state, 4);
  assert_string_equal(item, "appended");
}

static void
test_list_append_ordering(void **state)
{
  int err;
  char *item;

  err = list_append(*state, "e0");
  assert_int_equal(err, 0);
  err = list_append(*state, "e1");
  assert_int_equal(err, 0);
  err = list_append(*state, "e2");
  assert_int_equal(err, 0);

  item = list_getitem(*state, 0);
  assert_string_equal(item, "e0");
  item = list_getitem(*state, 1);
  assert_string_equal(item, "e1");
  item = list_getitem(*state, 2);
  assert_string_equal(item, "e2");
}

static void
test_list_push_ordering(void **state)
{
  int err;
  char *item;

  err = list_push(*state, "e0");
  assert_int_equal(err, 0);
  err = list_push(*state, "e1");
  assert_int_equal(err, 0);
  err = list_push(*state, "e2");
  assert_int_equal(err, 0);

  item = list_getitem(*state, 0);
  assert_string_equal(item, "e2");
  item = list_getitem(*state, 1);
  assert_string_equal(item, "e1");
  item = list_getitem(*state, 2);
  assert_string_equal(item, "e0");
}

int
main(int argc, char *argv[])
{
  const struct CMUnitTest tests[] = {
    /* list tests */
    cmocka_unit_test_setup_teardown(test_list_push_pop,
				    setup_list_empty,
				    teardown_list),
    cmocka_unit_test_setup_teardown(test_list_insert,
				    setup_list_empty,
				    teardown_list),
    cmocka_unit_test_setup_teardown(test_list_append_empty,
    				    setup_list_empty,
    				    teardown_list),
    cmocka_unit_test_setup_teardown(test_list_append_full,
    				    setup_list_3,
				    teardown_list),
    cmocka_unit_test_setup_teardown(test_list_append_ordering,
    				    setup_list_empty,
				    teardown_list),
    cmocka_unit_test_setup_teardown(test_list_push_ordering,
    				    setup_list_empty,
				    teardown_list),
  };
  return cmocka_run_group_tests(tests, NULL, NULL);
}
