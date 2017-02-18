
#include "list_test.h"

static void
test_list_insert_remove(void **state)
{
  int err;
  char *item;

  /* insert elements in the list */
  err = list_insert(*state, "0", 0);
  assert_int_equal(err, UTILS_OK);
  assert_int_equal(list_length(*state), 1);
  err = list_insert(*state, "1", 1);
  assert_int_equal(err, UTILS_OK);
  assert_int_equal(list_length(*state), 2);
  err = list_insert(*state, "2", 2);
  assert_int_equal(err, UTILS_OK);
  assert_int_equal(list_length(*state), 3);
  assert_int_equal(ctor_count, 3);
  assert_int_equal(dtor_count, 0);

  /* check removal of list elements in mixed order:
   * ["0", "1", "2"]
   * ["0", "2"]
   * ["0"]
   * []
   */
  item = list_remove(*state, 1);
  assert_non_null(item);
  assert_string_equal(item, "1");
  assert_int_equal(list_length(*state), 2);
  
  item = list_remove(*state, 1);
  assert_non_null(item);
  assert_string_equal(item, "2");
  assert_int_equal(list_length(*state), 1);

  /* remove out-of-bound item fails */
  item = list_remove(*state, 1);
  assert_null(item);
  assert_int_equal(list_length(*state), 1);
  
  item = list_remove(*state, 0);
  assert_non_null(item);
  assert_string_equal(item, "0");
  assert_int_equal(ctor_count, 3);
  /* dtors are not called because the remove 
   * does not deallocate the item (see delete)
   */
  assert_int_equal(dtor_count, 0);
}

static void
test_list_delete(void **state)
{
  int err;
  list_t lst = *state;

  /* delete elements for an initialised 4-element list 
   * delete in mixed order
   * [0, 1, 2, 3] (delete from middle)
   * [0, 1, 3] (delete from tail)
   * [0, 1] (delete from head)
   * [1] (delete from head)
   */
  assert_int_equal(list_length(lst), 4);

  err = list_delete(lst, 2);
  assert_int_equal(err, UTILS_OK);
  assert_int_equal(list_length(lst), 3);
  assert_int_equal(dtor_count, 1);
  
  err = list_delete(lst, 2);
  assert_int_equal(err, UTILS_OK);
  assert_int_equal(list_length(lst), 2);
  assert_int_equal(dtor_count, 2);

  /* delete unexisting fails */
  err = list_delete(lst, 2);
  assert_int_equal(err, UTILS_ERROR);
  assert_int_equal(list_length(lst), 2);
  assert_int_equal(dtor_count, 2);

  err = list_delete(lst, 1);
  assert_int_equal(err, UTILS_OK);
  assert_int_equal(list_length(lst), 1);
  assert_int_equal(dtor_count, 3);

  err = list_delete(lst, 0);
  assert_int_equal(err, UTILS_OK);
  assert_int_equal(list_length(lst), 0);
  assert_int_equal(dtor_count, 4);
}

/**
 * Try to destroy a list with only a single element
 * and check that the destructor is called.
 */
static void
test_list_destroy_one(void **state)
{
  int err;

  dtor_count = 0;

  err = list_insert(*state, "0", 0);
  assert_int_equal(err, UTILS_OK);
  assert_int_equal(list_length(*state), 1);
  assert_int_equal(dtor_count, 0);

  err = list_destroy(*state);
  assert_int_equal(err, UTILS_OK);
  assert_int_equal(dtor_count, 1);
}

int
main(int argc, char *argv[])
{
  const struct CMUnitTest tests[] = {
    /* list tests */
    cmocka_unit_test_setup_teardown(test_list_delete,
    				    setup_list_3,
    				    teardown_list),
    cmocka_unit_test_setup_teardown(test_list_insert_remove,
				    setup_list_empty,
				    teardown_list),
    cmocka_unit_test_setup(test_list_destroy_one, setup_list_empty),
  };
  return cmocka_run_group_tests(tests, NULL, NULL);
}
