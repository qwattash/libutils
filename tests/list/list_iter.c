
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

static void
test_list_iter_remove_first(void **state)
{
  /* try to remove an item in the list using the iterator interface */
  list_iter_struct_t iter;
  char *item_data;
  int rc;

  rc = list_iter_init(*state, &iter);
  assert_int_equal(rc, UTILS_OK);

  /* remove first item */
  list_item_t item = list_iter_item(&iter);
  assert_ptr_not_equal(item, NULL);
  item_data = list_item_remove(*state, item);
  assert_ptr_not_equal(item_data, NULL);
  assert_string_equal(item_data, "0");

  /* check postconditions */
  assert_int_equal(list_length(*state), 3);
  assert_string_equal(list_get(*state, 0), "1");
  assert_string_equal(list_get(*state, 1), "2");
  assert_string_equal(list_get(*state, 2), "3");
}

static void
test_list_iter_remove_mid(void **state)
{
  /* try to remove an item in the list using the iterator interface */
  list_iter_struct_t iter;
  char *item_data;
  int rc;

  rc = list_iter_init(*state, &iter);
  assert_int_equal(rc, UTILS_OK);

  /* remove second item */
  rc = list_iter_next(&iter);
  assert_int_equal(rc, UTILS_OK);
  list_item_t item = list_iter_item(&iter);
  assert_ptr_not_equal(item, NULL);
  item_data = list_item_remove(*state, item);
  assert_ptr_not_equal(item_data, NULL);
  assert_string_equal(item_data, "1");

  /* check postconditions */
  assert_int_equal(list_length(*state), 3);
  assert_string_equal(list_get(*state, 0), "0");
  assert_string_equal(list_get(*state, 1), "2");
  assert_string_equal(list_get(*state, 2), "3");
}

static void
test_list_iter_remove_last(void **state)
{
  /* try to remove an item in the list using the iterator interface */
  list_iter_struct_t iter;
  char *item_data;
  int rc;

  rc = list_iter_init(*state, &iter);
  assert_int_equal(rc, UTILS_OK);

  /* remove last item */
  rc = list_iter_seek(&iter, 3);
  assert_int_equal(rc, UTILS_OK);
  list_item_t item = list_iter_item(&iter);
  assert_ptr_not_equal(item, NULL);
  item_data = list_item_remove(*state, item);
  assert_ptr_not_equal(item_data, NULL);
  assert_string_equal(item_data, "3");

  /* check postconditions */
  assert_int_equal(list_length(*state), 3);
  assert_string_equal(list_get(*state, 0), "0");
  assert_string_equal(list_get(*state, 1), "1");
  assert_string_equal(list_get(*state, 2), "2");
}
  
static void
test_list_iter_add(void **state)
{

}

static void
test_list_iter_single(void **state)
{
  /* iterate single element list */
  list_iter_struct_t iter;
  char *item_data;
  int rc;
  bool is_end;

  rc = list_iter_init(*state, &iter);
  assert_int_equal(rc, UTILS_OK);

  /* before calling next */
  is_end = list_iter_end(&iter);
  assert_false(is_end);
  item_data = list_iter_data(&iter);
  assert_ptr_not_equal(item_data, NULL);
  assert_string_equal(item_data, "0");

  /* iter end */
  rc = list_iter_next(&iter);
  assert_int_equal(rc, UTILS_OK);
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
    cmocka_unit_test_setup_teardown(test_list_iter_remove_first,
    				    setup_list_3,
    				    teardown_list),
    cmocka_unit_test_setup_teardown(test_list_iter_remove_mid,
				    setup_list_3,
    				    teardown_list),
    cmocka_unit_test_setup_teardown(test_list_iter_remove_last,
    				    setup_list_3,
    				    teardown_list),
    cmocka_unit_test_setup_teardown(test_list_iter_single,
				    setup_list_1,
    				    teardown_list),
  };
  return cmocka_run_group_tests(tests, NULL, NULL);
}
