
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include <string.h>

#include "libutils/error.h"
#include "libutils/list.h"

#ifndef LIST_TEST_H
#define LIST_TEST_H

/* count calls to list item ctor and dtor */
static int ctor_count = 0;
static int dtor_count = 0;
static int walk_count = 0;

static int
ctor(void **item, void *data)
{
  ctor_count++;
  *item = data;
  return UTILS_OK;
}

static int
dtor(void *data)
{
  dtor_count++;
  return UTILS_OK;
}

static int
list_walk_cbk(void *item, void *args)
{
  walk_count++;
  return UTILS_OK;
}

static int
setup_list_empty(void **state)
{
  list_t lst;
  int err;

  ctor_count = 0;
  dtor_count = 0;
  walk_count = 0;

  err = list_init(&lst, ctor, dtor);
  if (err)
    return err;
  *state = lst;
  return 0;
}

static int
setup_list_3(void **state)
{
  list_t lst;
  int err;

  ctor_count = 0;
  dtor_count = 0;

  err = list_init(&lst, ctor, dtor);
  if (err)
    return err;
  err = list_push(lst, "3");
  if (err)
    return err;
  err = list_push(lst, "2");
  if (err)
    return err;
  err = list_push(lst, "1");
  if (err)
    return err;
  err = list_push(lst, "0");
  if (err)
    return err;
  *state = lst;
  return 0;
}

static int
setup_list_1(void **state)
{
  list_t lst;
  int err;

  ctor_count = 0;
  dtor_count = 0;

  err = list_init(&lst, ctor, dtor);
  if (err)
    return err;
  err = list_push(lst, "0");
  if (err)
    return err;
  *state = lst;
  return 0;
}

static int
teardown_list(void **state)
{
  int err;
  err = list_destroy(*state);
  return err;
}

#endif
