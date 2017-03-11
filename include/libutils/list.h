/**
 * @file
 * Generic list implementation
 */

#ifndef UTILS_LIST_H
#define UTILS_LIST_H

#include <stdbool.h>

/* Opaque types and data structures */

/**
 * Opaque list handle
 */
struct list_handle;
typedef struct list_handle * list_t;

/** 
 * Opaque list iterator structure
 */
struct list_iterator {
  struct list_handle *list;
  struct list_item *cursor;
  struct list_item *guard;
  bool end;
};
typedef struct list_iterator list_iter_struct_t;
typedef struct list_iterator * list_iter_t;

/**
 * Opaque list item handle. The functions that
 * operate on item handles perform faster operations
 * in O(1) time because the item handle retains
 * internal list information. Operations that accept
 * indexes and list data pointers are O(n) because
 * they need to scan the list to find the list item
 * handle for the data/index first.
 */
struct list_item;
typedef struct list_item * list_item_t;


/**
 * Callback used for constructor, destructors and walking
 */
typedef int (*list_ctor_t)(void **itm_data, void *data);
typedef int (*list_dtor_t)(void *itm_data);
typedef int (*list_cbk_t)(void *itm_data, void *args);
typedef int (*list_item_cbk_t)(list_item_t itm, void *args);

/* list setup API functions */

/**
 * initialise list handle with per-item constructor and
 * destructor.
 * @param[in]: handle pointer to a list handle
 * @param[in]: ctor item constructor callback
 * @param[in]: dtor item destructor callback
 * @return: utils error code
 */
int list_init(list_t *handle, list_ctor_t ctor, list_dtor_t dtor);

/**
 * Deallocate list, the destructor is called for each list
 * item.
 * @param[in]: handle to a list handle
 * @return: utils error code
 */
int list_destroy(list_t handle);

/* list data API functions */

/**
 * Push generic item to list
 * @param[in,out] handle: list handle
 * @param[in] data: data pointer to push
 * @return: utils error code
 */
int list_push(list_t handle, void *data);

/**
 * Pop generic item from list
 * @param[in,out] handle: list handle
 * @return: data pointer or NULL
 */
void* list_pop(list_t handle);

/**
 * Get length of the list
 * @param[in] handle: list handle
 * @return: length of the list or negative error value
 */
int list_length(list_t handle);

/**
 * Insert generic item to list at given position, the list is extended until
 * the requested position is reached.
 * @param[in,out] handle: list handle
 * @param[in] data: data pointer to push
 * @param[in] position: index in the list where data is inserted
 * @return: utils error code
 */
int list_insert(list_t handle, void *data, int position);

/**
 * Remove generic item from list at given position
 * @param[in,out] handle: list handle
 * @param[in] position: index in the list from which data is removed
 * @return: data at given position or NULL
 */
void* list_remove(list_t handle, int position);

/**
 * Get generic item from list at given position
 * @param[in,out] handle: list handle
 * @param[in] position: index in the list from which data is taken
 * @return: data at given position or NULL
 */
void* list_get(list_t handle, int position);

/**
 * Get index of an element in the list
 * @param[in] handle: list handle
 * @param[in] data: data object to search
 * @return: the item index or negative value if the item is not found
 */
int list_indexof(list_t handle, void *data);

/**
 * Remove and deallocate generic item from list at given position
 * @param[in,out] handle: list handle
 * @param[in] position: index in the list from which data is removed
 * @return: utils error code
 */
int list_delete(list_t handle, int position);

/**
 * Walk list executing given callback, if the callback returns
 * UTILS_ITER_STOP the iteration stops and no error is returned,
 * if the callback returns an error code the iteration stops 
 * and the error is propagated.
 * @param[in] handle: list handle to iterate
 * @param[in] cbk: callback to be run for each item
 * @param[in,out] args: extra arguments given to the callback
 * @return: utils error code
 */
int list_walk(list_t handle, list_cbk_t cbk, void *args);

/**
 * Append element to the end of the list
 * @param[in] handle: pointer to a list handle
 * @param[in] data: the data to append
 * @return: utils error code
 */
int list_append(list_t handle, void *data);

/* list iterator API functions */

/**
 * Get an iterator for the list
 * @param[in] handle: pointer to a list handle
 * @return: list iterator handle or NULL
 */
list_iter_t list_iter(list_t handle);

/**
 * Free an iterator
 * @param[in] iter: iterator handle to free.
 * @return: zero on success, negative error value
 */
int list_iter_free(list_iter_t iter);

/**
 * Advance the iterator.
 * @param[in] iter: the iterator handle
 * @return: zero on success, negative error value
 */
int list_iter_next(list_iter_t iter);

/**
 * Get next data object in the list from iterator
 * XXX the iterator can be made deletion-safe if it stores
 * a pointer to the head pointer in struct list_handle or a
 * pointer to the whole list_handle instead of a pointer to
 * the head element.
 * @param[in,out] iter: iterator handle
 * @return: data pointer or NULL
 */
void * list_iter_data(list_iter_t iter);

/**
 * Check if the iterator has reached the end
 * @param[in] iter: iterator handle
 * @return: bool, true if the iterator has finished
 */
bool list_iter_end(list_iter_t iter);

/**
 * Seek iterator to the given index
 * @param[in] iter: iterator handle
 * @param[in] index: the index to seek to
 * @return: zero on success, negative error value
 */
int list_iter_seek(list_iter_t iter, int index);

/**
 * Initialize a static iterator struct so
 * that no memory management is involved.
 * @param[in] handle: the list to iterate
 * @param[in,out] iter: iterator handle
 * @return: zero on success, error value on failure
 */
int list_iter_init(list_t handle, list_iter_t iter);

/* list item handle API functions */

/**
 * Get data object associated with a list item
 * @param[in] item: list item handle
 * @return: data contained in the item
 */
void * list_item_getdata(list_item_t item);

/**
 * Remove generic item from list at given position
 * @param[in] item: list item handle
 * @return: data contained by the item
 */
void * list_item_remove(list_t handle, list_item_t item);

/**
 * Get item handle for the item at the given position
 * @param[in,out] handle: list handle
 * @param[in] position: index in the list from which the item is taken
 * @return: list item at given position or NULL
 */
list_item_t list_item_get(list_t handle, int position);

/**
 * Remove and deallocate item from list at given position
 * @param[in,out] item: list item handle to delete
 * @return: utils error code
 */
int list_item_delete(list_t handle, list_item_t item);

/**
 * Walk list executing given callback, if the callback returns
 * UTILS_ITER_STOP the iteration stops and no error is returned,
 * if the callback returns an error code iteration stops and the
 * error code is propagated.
 * @param[in] handle: handle of the list to iterate
 * @param[in] cbk: callback to be run for each item
 * @param[in,out] args: extra arguments given to the callback
 * @return: utils error code
 */
int list_item_walk(list_t handle, list_item_cbk_t cbk, void *args);

/**
 * Get next item in the list from iterator
 * @param[in,out] iter: iterator handle
 * @return: item pointer or NULL
 */
list_item_t list_iter_item(list_iter_t iter);

#endif /* UTILS_LIST_H */
