#ifndef NOAM_BUFFER_H
#define NOAM_BUFFER_H

#include "noam_utility.h"

/* noam_buffer struct: represents a dynamic buffer
 *
 * data: pointer to internal buffer
 * length: number of elements
 * size: capacity of the buffer
 * chunk: size of value type in bytes
 * release: destructor for complex types */
typedef struct {
    void*             data;
    size_t            length;
    size_t            size;
    size_t            chunk;
    noam_release_func release;
} noam_buffer;

/* noam_buffer_create, noam_buffer_createv: creates a buffer allocating a `chunk` memory for each cell
 *
 * extended version passes the destructor for complex types, NULL is passed otherwise
 * */
noam_buffer* noam_buffer_createv(size_t chunk, noam_release_func release);
noam_buffer* noam_buffer_create(size_t chunk);

/* noam_buffer_at: returns a pointer to data at specific position */
void* noam_buffer_at(noam_buffer* buffer, size_t index);

/* noam_buffer_first, noam_buffer_last: returns a pointer to the first and the last element respectively
 * NULL if buffer is empty */
void* noam_buffer_first(noam_buffer* buffer);
void* noam_buffer_last(noam_buffer* buffer);

/* noam_buffer_release: calls a destructor for each complex element and releases memory */
void noam_buffer_release(noam_buffer* buffer);

/* noam_buffer_grow: reallocates the buffer with new `size` */
void noam_buffer_grow(noam_buffer* buffer, size_t size);

/* noam_buffer_push: appends `data` to the buffer */
void noam_buffer_push(noam_buffer* buffer, const void* data);

/* noam_buffer_append: appends a list of elements to the buffer */
void noam_buffer_append(noam_buffer* buffer, const void* data, size_t length);

/* noam_buffer_merge: merges two buffers storing the result in the first one */
void noam_buffer_merge(noam_buffer* buffer, noam_buffer* other);

/* noam_buffer_empty: checks if the buffer is empty */
int noam_buffer_empty(noam_buffer* buffer);

/* noam_buffer_clear: calls a destructor for each complex element and reallocates the buffer */
void noam_buffer_clear(noam_buffer* buffer);

/* noam_buffer_copy: returns a copy of a buffer */
noam_buffer* noam_buffer_copy(noam_buffer* buffer);

#endif //NOAM_BUFFER_H
