/**
 * @file list.c
 * implementation of a generic list
 * @author qwattash
 */


#include <stdlib.h>
#include <stdio.h>

#include "list.h"

#if 0 /* disabled due to cmocka bug */
#ifdef UNITTEST
extern void* _test_malloc(const size_t size, const char *file, const int line);
extern void _test_free(const void *ptr, const char *file, const int line);

#define malloc(size) _test_malloc(size, __FILE__, __LINE__)
#define free(ptr) _test_free(ptr, __FILE__, __LINE__)
#endif
#endif

/* XXX change this to E_ARG once we have error.h */
#define ASSERT_HANDLE_VALID(hnd) if (hnd == NULL) return -1
#define ASSERT_HANDLE_VALID_PTR(hnd) if (hnd == NULL) return NULL

/**
 * simple generic list structure
 */
struct list_item {
  struct list_item *next;
  void *data;
};

struct list_handle {
  struct list_item *base;
  list_ctor_t ctor;
  list_dtor_t dtor;
  size_t len;
};

int
list_init(list_t *phandle, list_ctor_t ctor, list_dtor_t dtor)
{
  list_t handle;

  if (phandle == NULL)
    return -1; /* XXX change this to E_SETUP once we have error.h */
  *phandle = malloc(sizeof(struct list_handle));
  if (*phandle == NULL)
    return -1; /* XXX change this to E_ALLOC once we have error.h */
  handle = *phandle;
  handle->base = NULL;
  handle->len = 0;
  handle->ctor = ctor;
  handle->dtor = dtor;
  return 0;
}

int
list_destroy(list_t handle)
{
  struct list_item *tmp;

  ASSERT_HANDLE_VALID(handle);
  
  while (handle->base != NULL) {
    tmp = handle->base;
    handle->base = handle->base->next;
    if (handle->dtor != NULL)
      handle->dtor(tmp->data);
    free(tmp);    
  }
  free(handle);
  return 0;
}

int
list_length(list_t handle)
{
  ASSERT_HANDLE_VALID(handle);
  return handle->len;
}

int
list_walk(list_t handle, list_cbk_t cbk, void *args)
{
  struct list_item *tmp;
  int err;
  
  ASSERT_HANDLE_VALID(handle);
  if (cbk == NULL)
    return -1; /* XXX change to error.h */
  tmp = handle->base;
  while (tmp != NULL) {
    err = cbk(tmp->data, args);
    if (err < 0)
      return err;
    if (err)
      return 0;
    tmp = tmp->next;
  }
  return 0;
}

int
list_push(list_t handle, void *data)
{
  return list_insert(handle, data, 0);
}

void*
list_pop(list_t handle)
{
  return list_remove(handle, 0);
}

int
list_insert(list_t handle, void *data, int position)
{
  struct list_item *current;
  struct list_item *previous;
  int index = 0;

  ASSERT_HANDLE_VALID(handle);

  current = handle->base;
  previous = handle->base;
  
  while (index < position) {
    if (current == NULL) {
      //create missing element with null data
      struct list_item *new = malloc(sizeof(struct list_item));
      if (new == NULL)
	return -1;
      if (handle->ctor != NULL)
	handle->ctor(&new->data, NULL);
      else
	new->data = NULL;
      
      if (current == handle->base) {
	//base is null
	handle->base = new;
	current = handle->base;
      }
      else {
	previous->next = new;
	current = new;
      }
      current->next = NULL;
      current->data = NULL;
      handle->len++;
    }
    previous = current;
    current = current->next;
    index++;
  }
  // position reached
  struct list_item *new = malloc(sizeof(struct list_item));
  if (new == NULL)
    return -1;
  if (handle->ctor != NULL)
    handle->ctor(&new->data, data);
  else
    new->data = data;
  
  if (current == handle->base)
    handle->base = new;
  else
    previous->next = new;
  new->next = current;
  handle->len++;
  return 0;
}

void*
list_remove(list_t handle, int position)
{
  struct list_item *current;
  struct list_item *previous;
  void *data;
  int index = 0;

  ASSERT_HANDLE_VALID_PTR(handle);
  /* check empty list */
  if (handle->base == NULL)
    return NULL;

  current = handle->base;
  previous = handle->base;  
  while (index < position) {
    previous = current;
    current = current->next;
    index++;
    // check for unexisting position
    if (current == NULL)
      return NULL;
  }
  // position reached
  if (current == handle->base)
    handle->base = current->next;
  else
    previous->next = current->next;
  data = current->data;
  free(current);
  handle->len--;
  return data;
}

void*
list_getitem(list_t handle, int position)
{
  struct list_item *current;
  int index = 0;

  ASSERT_HANDLE_VALID_PTR(handle);

  current = handle->base;
  if (current == NULL)
    return NULL;
  
  while (index < position) {
      current = current->next;
      index++;
      // check for unexisting position
      if (current == NULL) return NULL;
  }
  return current->data;
}

int
list_delete(list_t handle, int position)
{
  void *data;
  
  ASSERT_HANDLE_VALID(handle);
  
  data = list_remove(handle, position);
  if (data == NULL)
    return -1; /* XXX: convert to error.h */
  
  if (handle->dtor != NULL)
    handle->dtor(data);
  return 0;
}

int
list_append(list_t handle, void *data)
{
  struct list_item *current;
  struct list_item *previous;

  ASSERT_HANDLE_VALID(handle);
  
  current = handle->base;
  previous = handle->base;
  while (current != NULL) {
      previous = current;
      current = current->next;
  }
  struct list_item *new = malloc(sizeof(struct list_item));
  if (new == NULL)
    return -1;
  if (handle->ctor != NULL)
    handle->ctor(&new->data, data);
  else
    new->data = data;
  
  if (current == handle->base)
    handle->base = new; /* create head */
  else
    previous->next = new;
  new->next = NULL;
  handle->len++;
  return 0;
}
