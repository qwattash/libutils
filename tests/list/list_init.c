
#include "list_test.h"

static void
test_list_inv_hnd(void **state)
{
  char data;
  
  int err;
  void *perr;

  /* invalid list handle to all list functions */
  err = list_init(NULL, NULL, NULL);
  assert_int_equal(err, UTILS_ERROR);
  err = list_destroy(NULL);
  assert_int_equal(err, UTILS_ERROR);
  err = list_walk(NULL, &list_walk_cbk, NULL);
  assert_int_equal(err, UTILS_ERROR);
  err = list_push(NULL, &data);
  assert_int_equal(err, UTILS_ERROR);
  err = list_insert(NULL, &data, 0);
  assert_int_equal(err, UTILS_ERROR);
  err = list_delete(NULL, 0);
  assert_int_equal(err, UTILS_ERROR);

  perr = list_pop(NULL);
  assert_ptr_equal(perr, NULL);
  perr = list_remove(NULL, 0);
  assert_ptr_equal(perr, NULL);
  perr = list_get(NULL, 0);
  assert_ptr_equal(perr, NULL);
}

/* test list initialization */
static void
test_list_init_destroy(void **state)
{
  list_t lst = NULL;
  int err;
  /* check list initialization and destroy */
  err = list_init(&lst, NULL, NULL);
  assert_ptr_not_equal(lst, NULL);
  assert_int_equal(err, UTILS_OK);
  err = list_destroy(lst);
  assert_int_equal(err, UTILS_OK);
}

static void
test_list_full_destroy(void **state)
{
  list_t lst;
  int err;
  /* check deallocation of non-empty list */
  err = list_init(&lst, NULL, NULL);
  assert_int_equal(err, UTILS_OK);
  assert_ptr_not_equal(lst, NULL);
  err = list_push(lst, "0");
  assert_int_equal(err, UTILS_OK);
  err = list_destroy(lst);
  assert_int_equal(err, UTILS_OK);
}

int
main(int argc, char *argv[])
{
  const struct CMUnitTest tests[] = {
    /* list tests */
    cmocka_unit_test(test_list_inv_hnd),
    cmocka_unit_test(test_list_init_destroy),
    cmocka_unit_test(test_list_full_destroy),
  };
  return cmocka_run_group_tests(tests, NULL, NULL);
}
