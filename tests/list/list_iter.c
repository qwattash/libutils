
#include "list_test.h"

static void
test_list_iter(void **state)
{
  list_iter_t iter;
  char *item_data;
  int rc;
  bool is_end;

  iter = list_iter(*state);
  assert_ptr_not_equal(iter, NULL);

  /* element 0 */
  is_end = list_iter_end(iter);
  assert_false(is_end);
  item_data = list_iter_data(iter);
  assert_ptr_not_equal(item_data, NULL);
  assert_string_equal(item_data, "0");

  rc = list_iter_next(iter);
  assert_int_equal(rc, UTILS_OK);

  /* element 1 */
  is_end = list_iter_end(iter);
  assert_false(is_end);
  item_data = list_iter_data(iter);
  assert_ptr_not_equal(item_data, NULL);
  assert_string_equal(item_data, "1");

  rc = list_iter_next(iter);
  assert_int_equal(rc, UTILS_OK);

  /* element 2 */
  is_end = list_iter_end(iter);
  assert_false(is_end);
  item_data = list_iter_data(iter);
  assert_ptr_not_equal(item_data, NULL);
  assert_string_equal(item_data, "2");

  rc = list_iter_next(iter);
  assert_int_equal(rc, UTILS_OK);

  /* element 3 */
  is_end = list_iter_end(iter);
  assert_false(is_end);
  item_data = list_iter_data(iter);
  assert_ptr_not_equal(item_data, NULL);
  assert_string_equal(item_data, "3");

  rc = list_iter_next(iter);
  assert_int_equal(rc, UTILS_OK);

  /* iter end */
  is_end = list_iter_end(iter);
  assert_true(is_end);
  item_data = list_iter_data(iter);
  assert_null(item_data);
  
  /* free */
  rc = list_iter_free(iter);
  assert_int_equal(rc, UTILS_OK);
}

static void
test_iter_empty(void **state)
{
  list_iter_struct_t iter;
  void *data;
  int rc;
  bool is_end;

  rc = list_iter_init(*state, &iter);
  assert_int_equal(rc, UTILS_OK);

  is_end = list_iter_end(&iter);
  assert_true(is_end);

  data = list_iter_data(&iter);
  assert_null(data);

  rc = list_iter_next(&iter);
  assert_int_equal(rc, UTILS_ERROR);
  
}

static void
test_list_static_iter(void **state)
{
  list_iter_struct_t iter;
  char *item_data;
  int rc;
  bool is_end;

  rc = list_iter_init(*state, &iter);
  assert_int_equal(rc, UTILS_OK);

  /* element 0 */
  is_end = list_iter_end(&iter);
  assert_false(is_end);
  item_data = list_iter_data(&iter);
  assert_ptr_not_equal(item_data, NULL);
  assert_string_equal(item_data, "0");

  rc = list_iter_next(&iter);
  assert_int_equal(rc, UTILS_OK);

  /* element 1 */
  is_end = list_iter_end(&iter);
  assert_false(is_end);
  item_data = list_iter_data(&iter);
  assert_ptr_not_equal(item_data, NULL);
  assert_string_equal(item_data, "1");

  rc = list_iter_next(&iter);
  assert_int_equal(rc, UTILS_OK);

  /* element 2 */
  is_end = list_iter_end(&iter);
  assert_false(is_end);
  item_data = list_iter_data(&iter);
  assert_ptr_not_equal(item_data, NULL);
  assert_string_equal(item_data, "2");

  rc = list_iter_next(&iter);
  assert_int_equal(rc, UTILS_OK);

  /* element 3 */
  is_end = list_iter_end(&iter);
  assert_false(is_end);
  item_data = list_iter_data(&iter);
  assert_ptr_not_equal(item_data, NULL);
  assert_string_equal(item_data, "3");

  rc = list_iter_next(&iter);
  assert_int_equal(rc, UTILS_OK);

  /* iter end */
  is_end = list_iter_end(&iter);
  assert_true(is_end);
  item_data = list_iter_data(&iter);
  assert_null(item_data);
}

int
main(int argc, char *argv[])
{
  const struct CMUnitTest tests[] = {
    cmocka_unit_test_setup_teardown(test_list_iter,
    				    setup_list_3,
    				    teardown_list),
    cmocka_unit_test_setup_teardown(test_list_static_iter,
    				    setup_list_3,
    				    teardown_list),
    cmocka_unit_test_setup_teardown(test_iter_empty,
    				    setup_list_empty,
    				    teardown_list),
  };
  return cmocka_run_group_tests(tests, NULL, NULL);
}
