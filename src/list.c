/**
 * Generic list implementation
 */

#include <stdlib.h>
#include <stdio.h>

#include "libutils/error.h"
#include "libutils/list.h"

#if 0 /* disabled due to cmocka bug */
#ifdef UNITTEST
extern void * _test_malloc(const size_t size, const char *file, const int line);
extern void _test_free(const void *ptr, const char *file, const int line);

#define malloc(size) _test_malloc(size, __FILE__, __LINE__)
#define free(ptr) _test_free(ptr, __FILE__, __LINE__)
#endif
#endif

#define ASSERT_HANDLE_VALID(hnd) if (hnd == NULL) return UTILS_ERROR
#define ASSERT_HANDLE_VALID_PTR(hnd) if (hnd == NULL) return NULL

/**
 * list item internal representation
 */
struct list_item {
  struct list_item *next;
  struct list_item *prev;
  void *data;
};

/**
 * list internal representation
 */
struct list_handle {
  struct list_item *base;
  list_ctor_t ctor;
  list_dtor_t dtor;
  size_t len;
};

static int list_do_walk(struct list_handle *handle, void *cbk,
			void *args, bool walk_data);

int
list_init(list_t *phandle, list_ctor_t ctor, list_dtor_t dtor)
{
  list_t handle;

  if (phandle == NULL)
    return UTILS_ERROR;
  *phandle = malloc(sizeof(struct list_handle));
  if (*phandle == NULL)
    return UTILS_ERROR;
  handle = *phandle;
  handle->base = NULL;
  handle->len = 0;
  handle->ctor = ctor;
  handle->dtor = dtor;
  return UTILS_OK;
}

int
list_destroy(list_t handle)
{
  struct list_item *curr, *next;
  ASSERT_HANDLE_VALID(handle);

  if (handle->base != NULL) {
    curr = handle->base;
    while (curr->next != handle->base) {
      if (handle->dtor != NULL)
	handle->dtor(curr->data);
      next = curr->next;
      free(curr);
      curr = next;
    }
  }
  free(handle);
  return UTILS_OK;
}

int
list_length(list_t handle)
{
  ASSERT_HANDLE_VALID(handle);
  return handle->len;
}

/* list data API */

int
list_walk(list_t handle, list_cbk_t cbk, void *args)
{
  return list_do_walk(handle, (void *)cbk, args, true);
}

int
list_push(list_t handle, void *data)
{
  return list_insert(handle, data, 0);
}

void *
list_pop(list_t handle)
{
  return list_remove(handle, 0);
}

int
list_insert(list_t handle, void *data, int position)
{
  struct list_item *current;
  struct list_item *new;
  size_t index;
  bool direction;

  ASSERT_HANDLE_VALID(handle);

  /* append empty elements to the end of the list
   * if position is past the list length
   */
  while (position > handle->len) {
    new = malloc(sizeof(struct list_item));
    if (new == NULL)
      return UTILS_ERROR;
    if (handle->ctor != NULL)
      handle->ctor(&new->data, NULL);
    else
      new->data = NULL;
    
    if (handle->base == NULL) {
      handle->base = new;
      new->next = new;
      new->prev = new;
    }
    else {
      current = handle->base->prev;
      current->next = new;
      new->prev = current;
      handle->base->prev = new;
      new->next = handle->base;
    }
    handle->len++;
  }

  /* create the actual new data item */
  new = malloc(sizeof(struct list_item));
  if (new == NULL)
    return UTILS_ERROR;
  if (handle->ctor != NULL)
    handle->ctor(&new->data, data);
  else
    new->data = data;
  
  if (handle->base == NULL) {
    handle->base = new;
    new->next = new;
    new->prev = new;
  }
  else {
    /* iterate to the position from the closest side */
    direction = ((handle->len / 2 - position) >= 0);
    index = direction ? 0 : handle->len;
    current = handle->base;
    while (index != position) {
      current = direction ? current->next : current->prev;
      index = direction ? index + 1 : index - 1;
    }
    new->next = current;
    new->prev = current->prev;
    current->prev = new;
    new->prev->next = new;
    if (index == 0)
      /* update list head if replacing the first element */
      handle->base = new;
  }
  handle->len++;
  return UTILS_OK;
}

void *
list_remove(list_t handle, int position)
{
  struct list_item *target;
  void *data;

  ASSERT_HANDLE_VALID_PTR(handle);

  target = list_item_get(handle, position);
  if (target == NULL)
    return NULL;

  data = list_item_remove(handle, target);
  return data;
}

void *
list_get(list_t handle, int position)
{
  struct list_item *target;

  ASSERT_HANDLE_VALID_PTR(handle);

  target = list_item_get(handle, position);
  if (target == NULL)
    return NULL;
  return target->data;
}

int
list_indexof(list_t handle, void *data)
{
  struct list_iterator iter;
  void *item;
  int index;

  ASSERT_HANDLE_VALID(handle);

  index = 0;
  for (list_iter_init(handle, &iter); !list_iter_end(&iter);
       list_iter_next(&iter)) {
    item = list_iter_data(&iter);
    if (item == data)
      break;
    index++;
  }
  if (list_iter_end(&iter))
    return -1;
  return index;
}

int
list_delete(list_t handle, int position)
{
  void *data;
  
  ASSERT_HANDLE_VALID(handle);
  if (list_length(handle) <= position)
    return UTILS_ERROR;

  data = list_remove(handle, position);  
  if (handle->dtor != NULL)
    handle->dtor(data);
  return UTILS_OK;
}

int
list_append(list_t handle, void *data)
{
  ASSERT_HANDLE_VALID(handle);

  return list_insert(handle, data, handle->len);
}

/* list iterator API */

list_iter_t
list_iter(list_t handle)
{
  struct list_iterator *list_iter;

  ASSERT_HANDLE_VALID_PTR(handle);

  list_iter = malloc(sizeof(struct list_iterator));
  if (list_iter == NULL)
    return NULL;

  if (list_iter_init(handle, list_iter)) {
    free(list_iter);
    return NULL;
  }
  return list_iter;
}

int
list_iter_init(list_t handle, list_iter_t iter)
{
  ASSERT_HANDLE_VALID(handle);

  iter->list = handle;
  iter->cursor = handle->base;
  if (iter->cursor == NULL) {
    iter->end = true;
  }
  else {
    iter->end = false;
  }
  iter->guard = NULL;
  return UTILS_OK;
}

int
list_iter_free(list_iter_t iter)
{
  if (iter == NULL)
    return UTILS_ERROR;

  free(iter);
  return UTILS_OK;
}

int
list_iter_next(list_iter_t iter)
{
  struct list_item *item;
  
  if (iter == NULL)
    return UTILS_ERROR;
  if (iter->cursor == NULL)
    return UTILS_ERROR;

  iter->guard = iter->list->base;
  item = iter->cursor->next;
  if (item == iter->guard)
    iter->end = true;
  else
    iter->cursor = item;
  return UTILS_OK;
}

void *
list_iter_data(list_iter_t iter)
{
  struct list_item *item;

  item = list_iter_item(iter);
  if (item == NULL)
    return NULL;
  return item->data;
}

bool
list_iter_end(list_iter_t iter)
{
  if (iter == NULL)
    return true;

  if (iter->cursor == NULL)
    iter->end = true;
  else if (iter->cursor == iter->guard)
    iter->end = true;
  return iter->end;
}

int
list_iter_seek(list_iter_t iter, int index)
{
  struct list_item *curr;
  int position;

  if (iter == NULL)
    return UTILS_ERROR;
  if (iter->list->base == NULL) {
    return UTILS_ERROR;
  }
  if (index == 0)
    /* In this case the iterator resets and the guard item resets too */
    iter->guard = NULL;
  else
    iter->guard = iter->list->base;

  position = 0;
  curr = iter->list->base;

  while (curr != iter->guard && position < index) {
    curr = curr->next;
    position++;
  }

  if (curr == iter->guard) {
    /* Can not seek out of bound index */
    iter->end = true;
    return UTILS_ERROR;
  }

  iter->cursor = curr;
  iter->end = false;
  return UTILS_OK;
}

/* list item handles API */

void *
list_item_getdata(list_item_t item)
{
  return item->data;
}

void *
list_item_remove(list_t handle, list_item_t item)
{
  void *data;

  ASSERT_HANDLE_VALID_PTR(handle);

  data = item->data;
  if (handle->base == item) {
    /* check for single item to update the base correctly */
    if (item == item->next)
      handle->base = NULL;
    else
      handle->base = item->next;
  }
  if (item != item->next) {
    /* if there is more than one item update the pointers */
    item->prev->next = item->next;
    item->next->prev = item->prev;
  }
  free(item);
  handle->len--;
  return data;
}

list_item_t
list_item_get(list_t handle, int position)
{
  struct list_iterator iter;

  ASSERT_HANDLE_VALID_PTR(handle);

  for (list_iter_init(handle, &iter); !list_iter_end(&iter) && position > 0;
       list_iter_next(&iter))
    position--;
  if (list_iter_end(&iter))
    return NULL;
  return list_iter_item(&iter);
}

int
list_item_delete(list_t handle, list_item_t item)
{
  void *data;

  ASSERT_HANDLE_VALID(handle);

  data = list_item_remove(handle, item);
  if (handle->dtor != NULL)
    handle->dtor(data);
  return UTILS_OK;
}

int
list_item_walk(list_t handle, list_item_cbk_t cbk, void *args)
{
  return list_do_walk(handle, (void *)cbk, args, false);
}

list_item_t
list_iter_item(list_iter_t iter)
{
  if (iter == NULL)
    return NULL;
  if (iter->end)
    return NULL;
  return iter->cursor;
}

/**
 * Common list walk logic
 * 
 * @param[in] handle: the list handle to iterate
 * @param[in] cbk: callback, the type is determined by walk_data
 * @param[in,out] args: user-defined callback arguments
 * @param[in] walk_data: if true, the callback is invoked with the list
 * item data, if false the callback is invoked with the list item handle.
 * @return: util error code according to the list_walk documentation
 */
static int
list_do_walk(struct list_handle *handle, void *cbk, void *args, bool walk_data)
{
  struct list_iterator iter;
  list_cbk_t data_cbk;
  list_item_cbk_t item_cbk;
  int err;
  
  ASSERT_HANDLE_VALID(handle);

  if (cbk == NULL)
    return UTILS_ERROR;

  if (walk_data)
    data_cbk = cbk;
  else
    item_cbk = cbk;

  for (list_iter_init(handle, &iter); !list_iter_end(&iter);
       list_iter_next(&iter)) {
    if (walk_data)
      err = data_cbk(list_iter_data(&iter), args);
    else
      err = item_cbk(list_iter_item(&iter), args);

    if (err == UTILS_ITER_STOP)
      break;
    if (err != UTILS_OK)
      return UTILS_ERROR;
  }
  return UTILS_OK;
}
