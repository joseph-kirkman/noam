#ifndef NOAM_DICT_H
#define NOAM_DICT_H

#include "noam_utility.h"
#include "noam_buffer.h"

/* noam_dict_node: node of a dictionary in a chaining scheme
 *
 * data: holds a key:value pair, where key starts at the beginning and value = data + key_chunk
 * next: pointer to the next node in a bin
 * */
typedef struct noam_dict_node {
    char*                  data;
    struct noam_dict_node* next;
} noam_dict_node;

/* noam_hash_func: a prototype of a hash function used in dictionaries */
typedef size_t(*noam_hash_func)(const void*);

/* noam_cmp_func: a comparator for two keys, returns -1 if a < b, 0 if a == b, 1 if a > b */
typedef int(*noam_cmp_func)(const void*, const void*);

/* noam_dict struct: represents a hash map
 *
 * nodes: array of bins
 * length: number of elements
 * size: number of bins
 * key_chunk: size of key in bytes
 * val_chunk: size of value in bytes
 * hash: hash function for keys
 * cmp: comparator for keys
 * release: destructor for complex types
 * */
typedef struct {
    noam_dict_node*   nodes;
    size_t            length;
    size_t            size;
    size_t            key_chunk;
    size_t            val_chunk;
    noam_hash_func    hash;
    noam_cmp_func     cmp;
    noam_release_func release;
} noam_dict;

noam_dict* noam_dict_createv(size_t key_chunk, size_t val_chunk,
                             noam_hash_func hash, noam_cmp_func cmp,
                             noam_release_func release);

noam_dict* noam_dict_create(size_t key_chunk, size_t val_chunk, noam_hash_func hash);

void noam_dict_insert(noam_dict* dict, void* key, void* value);

noam_dict_node* noam_dict_find(noam_dict* dict, void* key);

void* noam_dict_value(noam_dict* dict, noam_dict_node* node);

void noam_dict_release(noam_dict* dict);

size_t noam_hash_string(const noam_buffer* str);

int noam_cmp_string(const noam_buffer* lhs, const noam_buffer* rhs);

#endif //NOAM_DICT_H
