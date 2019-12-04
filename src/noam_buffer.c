#include "noam_buffer.h"

#define NOAM_BUFFER_GROW_FACTOR 2

noam_buffer* noam_buffer_createv(size_t chunk, noam_release_func release){
    noam_buffer* buffer = malloc(sizeof(noam_buffer));
    buffer->data = malloc(chunk);
    memset(buffer->data, 0, chunk);
    buffer->length = 0;
    buffer->size = 1;
    buffer->chunk = chunk;
    buffer->release = release;
    return buffer;
}

noam_buffer* noam_buffer_create(size_t chunk){
    return noam_buffer_createv(chunk, NULL);
}

void* noam_buffer_at(noam_buffer* buffer, size_t index){
    return (unsigned char*)buffer->data + index * buffer->chunk;
}

void* noam_buffer_first(noam_buffer* buffer){
    if(!buffer->length){
        return NULL;
    }
    return buffer->data;
}

void* noam_buffer_last(noam_buffer* buffer){
    if(!buffer->length){
        return NULL;
    }
    return noam_buffer_at(buffer, buffer->length - 1);
}

void noam_buffer_release(noam_buffer* buffer){
    if(buffer->release){
        for(size_t i = 0; i < buffer->length; ++i){
            buffer->release(noam_buffer_at(buffer, i));
        }
    }
    free(buffer->data);
    free(buffer);
}

void noam_buffer_grow(noam_buffer* buffer, size_t size){
    buffer->data = realloc(buffer->data, size * buffer->chunk);
    buffer->size = size;
}

void noam_buffer_push(noam_buffer* buffer, const void* data){
    if(buffer->length >= buffer->size){
        noam_buffer_grow(buffer, NOAM_BUFFER_GROW_FACTOR * buffer->size);
    }
    memmove(noam_buffer_at(buffer, buffer->length), data, buffer->chunk);
    ++buffer->length;
}

void noam_buffer_append(noam_buffer* buffer, const void* data, size_t length){
    if(buffer->length + length >= buffer->size){
        noam_buffer_grow(buffer, NOAM_BUFFER_GROW_FACTOR * (buffer->length + length));
    }
    memmove(noam_buffer_at(buffer, buffer->length), data, length * buffer->chunk);
    buffer->length += length;
}

void noam_buffer_merge(noam_buffer* buffer, noam_buffer* other){
    noam_buffer_append(buffer, other->data, other->length);
}

int noam_buffer_empty(noam_buffer* buffer){
    return buffer->length == 0;
}

void noam_buffer_clear(noam_buffer* buffer){

    if(buffer->release){
        for(size_t i = 0; i < buffer->length; ++i){
            buffer->release(noam_buffer_at(buffer, i));
        }
    }

    buffer->data = realloc(buffer->data, buffer->chunk);
    buffer->length = 0;
    buffer->size = 1;
}

noam_buffer* noam_buffer_copy(noam_buffer* buffer){
    noam_buffer* copy = malloc(sizeof(noam_buffer));
    copy->data = malloc(buffer->chunk * buffer->size);
    memmove(copy->data, buffer->data, buffer->chunk * buffer->size);
    copy->length = buffer->length;
    copy->size = buffer->size;
    copy->chunk = buffer->chunk;
    copy->release = buffer->release;
    return copy;
}
