#include "noam_dict.h"

noam_dict* noam_dict_createv(size_t key_chunk, size_t val_chunk,
                             noam_hash_func hash, noam_cmp_func cmp,
                             noam_release_func release){
    noam_dict* dict = malloc(sizeof(noam_dict));
    dict->nodes = malloc(sizeof(noam_dict_node));
    memset(dict->nodes, 0, sizeof(noam_dict_node));
    dict->key_chunk = key_chunk;
    dict->val_chunk = val_chunk;
    dict->length = 0;
    dict->size = 1;
    dict->hash = hash;
    dict->cmp = cmp;
    dict->release = release;
    return dict;
}

noam_dict* noam_dict_create(size_t key_chunk, size_t val_chunk, noam_hash_func hash){
    return noam_dict_createv(key_chunk, val_chunk, hash, NULL, NULL);
}

noam_dict_node* noam_dict_node_create(noam_dict* dict, void* key, void* value){
    noam_dict_node* node = malloc(sizeof(node));
    node->data = malloc(dict->key_chunk + dict->val_chunk);
    if(key){
        memcpy(node->data, key, dict->key_chunk);
    }
    if(value){
        memcpy(node->data + dict->key_chunk, value, dict->val_chunk);
    }
    node->next = NULL;
    return node;
}

void noam_dict_insert(noam_dict* dict, void* key, void* value){
    //TODO: Rehash on purpose
    size_t hash = dict->hash(key) % dict->size;
    noam_dict_node* node = &dict->nodes[hash];

    if(dict->cmp){
        while(node->next){
            if(dict->cmp(node->next->data, key) == 0){
                memcpy(node->next->data + dict->key_chunk, value, dict->val_chunk);
                return;
            }
            node = node->next;
        }
    } else {
        while(node->next){
            if(memcmp(node->next->data, key, dict->key_chunk) == 0){
                memcpy(node->next->data + dict->key_chunk, value, dict->val_chunk);
                return;
            }
            node = node->next;
        }
    }

    if(!node->next){
        node->next = noam_dict_node_create(dict, key, value);
    }
}

noam_dict_node* noam_dict_find(noam_dict* dict, void* key){
    size_t hash = dict->hash(key) % dict->size;
    noam_dict_node* node = dict->nodes[hash].next;

    if(dict->cmp){
        while(node){
            if(dict->cmp(node->data, key) == 0)
                return node;
            node = node->next;
        }
        return NULL;
    } else {
        while(node){
            if(memcmp(node->data, key, dict->key_chunk) == 0)
                return node;
            node = node->next;
        }
        return NULL;
    }
}

void* noam_dict_value(noam_dict* dict, noam_dict_node* node){
    return node->data + dict->key_chunk;
}

void noam_dict_release(noam_dict* dict){
    if(dict->release){
        for(size_t i = 0; i < dict->size; ++i){
            noam_dict_node* node = dict->nodes[i].next;

            while(node){
                noam_dict_node* next = node->next;
                dict->release(node->data);
                free(node);
                node = next;
            }
        }
    }

    free(dict->nodes);
    free(dict);
}

size_t noam_hash_string(const noam_buffer* str){
    size_t hash = 5381;
    const char* s = str->data;

    while(*s){
        hash = ((hash << 5) + hash) + *(s++);
    }

    return hash;
}

int noam_cmp_string(const noam_buffer* lhs, const noam_buffer* rhs){
    if(!lhs && rhs)
        return -1;
    if(lhs && !rhs)
        return 1;
    return strcmp(lhs->data, rhs->data);
}