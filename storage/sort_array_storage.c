//
// Created by illibejiep on 12/27/16.
//

#include "sort_array_storage.h"

typedef struct _sort_array_storage {
    keys_storage base;
    char* array;
} sort_array_storage;

static size_t search(sort_array_storage* storage, char str[static 1]);
static bool add(sort_array_storage* _self, char str[static 1]);
static bool del(sort_array_storage* _self, char str[static 1]);
static bool has(sort_array_storage* _self, char str[static 1]);
static bool free_storage(sort_array_storage* _self);

keys_storage * sort_array_storage_create(size_t key_size)
{
    sort_array_storage* storage = emalloc(sizeof(sort_array_storage));
    //printf("created %p\n", storage);

    storage->base.count = 0;
    storage->base.key_size = key_size;
    storage->base.allocated = PAGE_NSIZE*key_size;
    storage->array = emalloc(storage->base.allocated);
    memset(storage->array, 0, storage->base.allocated);

    storage->base.add = (add_storage_method)add;
    storage->base.del = (del_storage_method)del;
    storage->base.has = (has_storage_method)has;
    storage->base.free_storage = (free_storage_storage_method)free_storage;
    //printf("add: %p\n", sort_array_storage_add);
    //printf("del: %p\n", sort_array_storage_del);
    //printf("has: %p\n", sort_array_storage_has);
    //printf("search: %p\n", sort_array_storage_search);

    return (keys_storage*)storage;
}

static size_t search(sort_array_storage* _self, char str[static 1])
{
    //printf("str:%.*s\n", _self->key_size, str);
    if (_self->base.count == 0) {
        return 0;
    }

    size_t left = 0;
    size_t right = _self->base.count-1;
    size_t mid = (left + right)/2;
    void* mid_ptr = _self->array + mid*_self->base.key_size;;

    //printf("%d, %d, %d\n", left, mid, right);

    while (left < right) {

        //printf("%d, %d, %d\n", left, mid, right);
        //printf("memcmp:%d\n", memcmp(mid_ptr, str, _self->key_size));

        //printf("checking1\n");
        if (memcmp(mid_ptr, str, _self->base.key_size) < 0) {
            left = mid < right ? mid+1 : right;
        } else if (memcmp(mid_ptr, str, _self->base.key_size) > 0) {
            right = mid > left ? mid-1 : left;
        } else {
            return mid;
        }

        mid = (left + right)/2;
        mid_ptr = _self->array + mid*_self->base.key_size;

        //printf("%d, %d, %d\n", left, mid, right);
    }

    //printf("%.*s <=> %.*s\n", _self->key_size, mid_ptr, _self->key_size, str);
    //printf("checking2: %d\n", memcmp(mid_ptr, str, _self->key_size));
    if (memcmp(mid_ptr, str, _self->base.key_size) < 0) {
        mid++;
    }

    //printf("end: %d, %d, %d\n", left, mid, right);

    return mid;
}

static bool add(sort_array_storage* _self, char str[static 1])
{
    if (_self->base.add != (add_storage_method)add) {
        keys_storage_errno = WRONG_STORAGE_OBJECT_ERRNO;
        return false;
    }

    // out of space?
    if (_self->base.count*_self->base.key_size >= _self->base.allocated) {
        _self->base.allocated += PAGE_NSIZE*_self->base.key_size;
        _self->array = erealloc(_self->array, _self->base.allocated);
        //memset(_self->mem + _self->count*_self->key_size, 0, _self->allocated - _self->count*_self->key_size);
        //printf("realloc to %d\n", _self->allocated);
        //printf("[ %p ; %p ]\n", _self->mem, _self->mem + _self->allocated);
    }

    size_t insert_idx = search(_self, str);
    //printf("%s <=> %s\n", found, str);
    //printf("memcmp: %d\n", memcmp(found, str, _self->key_size));

    void* insert_ptr = _self->array + insert_idx*_self->base.key_size;

    // duplicate key
    if (memcmp(insert_ptr, str, _self->base.key_size) == 0)
        return false;

    //printf("found %.*s at %d/%d : %d\n", _self->key_size, insert_ptr, insert_idx, _self->count, _self->allocated);

    void* tail_ptr = _self->array + (_self->base.key_size*_self->base.count);

    //printf("dest %d\n", dest - _self->mem);

    while (tail_ptr > insert_ptr) {
        //printf("shifting\n");
        memcpy(tail_ptr, tail_ptr - _self->base.key_size, _self->base.key_size);
        tail_ptr -= _self->base.key_size;
    }
    //printf("insertng\n");
    memcpy(insert_ptr, str, _self->base.key_size);

    _self->base.count++;

//    for (int i=0 ; i < _self->count*_self->key_size ; i++) {
//        char c = *(char*)(_self->mem+i);
//        if (c == 0)
//            c = ' ';
//
//        printf("%c", c);
//    }
//    printf("\n");

    return true;
}

static bool del(sort_array_storage* _self, char str[static 1])
{
    if (_self->base.del != (del_storage_method)del) {
        keys_storage_errno = WRONG_STORAGE_OBJECT_ERRNO;
        return false;
    }
    sort_array_storage* storage = (sort_array_storage*)_self;

    if (_self->base.count == 0)
        return false;

    size_t found = search(storage, str);
    void* found_ptr = _self->array + found*_self->base.key_size;

    if (memcmp(found_ptr, str, _self->base.key_size) != 0)
        return false;

    for (size_t i = found ; i < _self->base.count-1 ; i++) {
        void* i_ptr = _self->array + i*_self->base.key_size;
        memcpy(i_ptr, i_ptr + _self->base.key_size, _self->base.key_size);
    }

    void* last_ptr = _self->array + (_self->base.count-1)*_self->base.key_size;
    memset(last_ptr, 0, _self->base.key_size);

    _self->base.count--;

    // reduce array
    if (_self->base.count > 3*PAGE_NSIZE && _self->base.count*_self->base.key_size < _self->base.allocated - 2*PAGE_NSIZE*_self->base.key_size) {
        _self->base.allocated -= PAGE_NSIZE*_self->base.key_size;
        _self->array = erealloc(_self->array, _self->base.allocated);
        //memset(_self->mem + _self->count*_self->key_size, 0, _self->allocated-_self->count*_self->key_size);
    }

    return true;
}

static bool has(sort_array_storage* _self, char str[static 1])
{
    if (_self->base.has != (has_storage_method)has) {
        keys_storage_errno = WRONG_STORAGE_OBJECT_ERRNO;
        return false;
    }

    size_t found = search(_self, str);
    void* found_ptr = _self->array + found*_self->base.key_size;

    if (memcmp(found_ptr, str, _self->base.key_size) == 0)
        return true;

    return false;
}

static bool free_storage(sort_array_storage* _self)
{
    if (_self->base.free_storage != (free_storage_storage_method)free_storage) {
        keys_storage_errno = WRONG_STORAGE_OBJECT_ERRNO;
        return false;
    }
    
    efree(_self->array);
    _self->array = NULL;

    return true;
}