
#include "list_test.h"

static void
test_list_iter(void **state)
{
  list_iter_t iter;
  char *item_data;
  int rc;
  bool is_end;

  rc = list_iter(*state, &iter);
  assert_int_equal(rc, 0);

  /* element 0 */
  is_end = list_iter_end(iter);
  assert_false(is_end);
  rc = list_iter_item(iter, (void **)&item_data);
  assert_int_equal(rc, 0);
  assert_string_equal(item_data, "0");

  rc = list_iter_next(iter);
  assert_int_equal(rc, 0);

  /* element 1 */
  is_end = list_iter_end(iter);
  assert_false(is_end);
  rc = list_iter_item(iter, (void **)&item_data);
  assert_int_equal(rc, 0);
  assert_string_equal(item_data, "1");

  rc = list_iter_next(iter);
  assert_int_equal(rc, 0);

  /* element 2 */
  is_end = list_iter_end(iter);
  assert_false(is_end);
  rc = list_iter_item(iter, (void **)&item_data);
  assert_int_equal(rc, 0);
  assert_string_equal(item_data, "2");

  rc = list_iter_next(iter);
  assert_int_equal(rc, 0);

  /* element 3 */
  is_end = list_iter_end(iter);
  assert_false(is_end);
  rc = list_iter_item(iter, (void **)&item_data);
  assert_int_equal(rc, 0);
  assert_string_equal(item_data, "3");

  rc = list_iter_next(iter);
  assert_int_equal(rc, 0);

  /* iter end */
  is_end = list_iter_end(iter);
  assert_true(is_end);
  rc = list_iter_item(iter, (void **)&item_data);
  assert_int_equal(rc, -1);
  
  /* free */
  rc = list_iter_free(iter);
  assert_int_equal(rc, 0);
}


int
main(int argc, char *argv[])
{
  const struct CMUnitTest tests[] = {
    cmocka_unit_test_setup_teardown(test_list_iter,
    				    setup_list_3,
    				    teardown_list),
  };
  return cmocka_run_group_tests(tests, NULL, NULL);
}
