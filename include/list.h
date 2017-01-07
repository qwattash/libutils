/**
 * @file list.h
 * Generic list implementation
 * @author qwattash
 */

#ifndef LIST_H
#define LIST_H

#include <stdbool.h>

/**
 * Opaque list handle
 */
struct list_handle;
typedef struct list_handle * list_t;

struct list_iterator;
typedef struct list_iterator * list_iter_t;

/**
 * Callback used for constructor, destructors and walking
 */
typedef int (*list_ctor_t)(void **itm_data, void *data);
typedef int (*list_dtor_t)(void *itm_data);
typedef int (*list_cbk_t)(void *itm_data, void *args);

/**
 * Push generic item to list
 * @param[in,out]: handle to a list handle
 * @param[in]: data data pointer to push
 * @return: zero on success, error code on failure
 */
int list_push(list_t handle, void *data);

/**
 * Pop generic item from list
 * @param[in,out]: handle to a list handle
 * @return: data pointer or NULL
 */
void* list_pop(list_t handle);

/**
 * Get length of the list
 * @param[in]: handle list handle
 * @return:length of the list or negative error value
 */
int list_length(list_t handle);

/**
 * Insert generic item to list at given position, the list is extended until
 * the requested position is reached.
 * @param[in,out]: handle to a list handle
 * @param[in]: data data pointer to push
 * @param[in]: position index in the list where data is inserted
 * @return: true on success, false on failure
 */
int list_insert(list_t handle, void *data, int position);

/**
 * Remove generic item from list at given position
 * @param[in,out]: handle to a list handle
 * @param[in]: position index in the list from which data is removed
 * @return: data at given position or NULL
 */
void* list_remove(list_t handle, int position);

/**
 * Get generic item from list at given position
 * @param[in,out]: handle to a list handle
 * @param[in]: position index in the list from which data is taken
 * @return: data at given position or NULL
 */
void* list_getitem(list_t handle, int position);

/**
 * Remove and deallocate generic item from list at given position
 * @param[in,out]: handle to a list handle
 * @param[in]: position index in the list from which data is removed
 * @return: zero on success
 */
int list_delete(list_t handle, int position);

/**
 * Walk list executing given callback, if the callback returns
 * a negative value the iteration stops and returns the error code,
 * if the callback returns a non-zero positive value the iteration
 * stops returning zero.
 * @param[in]: handle to a list handle
 * @param[in]: cbk callback to be run for each item
 * @param[in]: args extra arguments given to the callback
 * @return: zero on success, negative error value
 */
int list_walk(list_t handle, list_cbk_t cbk, void *args);

/**
 * Deallocate list, the destructor is called for each list
 * item.
 * @param[in]: handle to a list handle
 * @return: zero on success, negative error value
 */
int list_destroy(list_t handle);

/**
 * initialise list handle with per-item constructor and
 * destructor.
 * @param[in]: handle pointer to a list handle
 * @param[in]: ctor item constructor callback
 * @param[in]: dtor item destructor callback
 * @return: zero on success, negative error value
 */
int list_init(list_t *handle, list_ctor_t ctor, list_dtor_t dtor);

/**
 * Append element to the end of the list
 * @param[in] handle: pointer to a list handle
 * @param[in] data: the data to append
 * @return: zero on success, negative error value
 */
int list_append(list_t handle, void *data);

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
 * Get next item in the list from iterator
 * XXX the iterator can be made deletion-safe if it stores
 * a pointer to the head pointer in struct list_handle or a
 * pointer to the whole list_handle instead of a pointer to
 * the head element.
 * @param[in,out] iter: iterator handle
 * @return: item pointer or NULL
 */
void *list_iter_item(list_iter_t iter);

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

#endif
