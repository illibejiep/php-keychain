//
// Created by illibejiep on 12/27/16.
//

#include "open_addressing_storage.h"

#define INIT_NSIZE 1024

typedef struct {
    size_t size;
    char array[1];
} table;

typedef struct {
    keys_storage base;
    table* table;
} open_addressing_storage;

static bool add(open_addressing_storage* _self, char str[static 1]);
static bool del(open_addressing_storage* _self, char str[static 1]);
static bool has(open_addressing_storage* _self, char str[static 1]);
static bool free_storage(open_addressing_storage* _self);

static size_t hash(char str[static 1], size_t  len);
static size_t rehash(size_t hash, char str[static 1], size_t len);

keys_storage * open_addressing_storage_create(size_t key_size)
{
    open_addressing_storage* storage = emalloc(sizeof(open_addressing_storage));

    storage->base.error = NULL;
    storage->base.count = 0;
    storage->base.key_size = key_size;
    storage->base.allocated = INIT_NSIZE*key_size;
    storage->table = emalloc(sizeof(table) + INIT_NSIZE*key_size);
    memset(storage->table, 0, sizeof(table) + INIT_NSIZE*key_size);
    storage->table->size = INIT_NSIZE;

    storage->base.add = (add_storage_method)add;
    storage->base.del = (del_storage_method)del;
    storage->base.has = (has_storage_method)has;
    storage->base.free_storage = (free_storage_storage_method)free_storage;

    return (keys_storage*)storage;
}

static size_t hash(char str[static 1], size_t len)
{
    size_t hash = 5381;
    for (size_t i=0; i < len ; i++)
        hash = ((hash << 5) + hash) + str[i]; /* hash * 33 + c */

    return hash % 18446744073709551557;
}

static size_t rehash(size_t hash, char str[static 1], size_t len)
{
    size_t new_hash = 0;
    for (size_t i=0; i < len ; i++)
        new_hash = new_hash*5 + str[i]; /* hash * 33 + c */

    return  hash + new_hash;
}

static inline bool is_available(table* table, size_t pos, size_t key_size)
{
    for (size_t i = 0 ; i < key_size ; i++)
        if (table->array[pos*key_size+i] != 0 && table->array[pos*key_size+i] != 255)
            return false;

    return true;
}

// for search
static inline bool is_empty(table* table, size_t pos, size_t key_size)
{
    for (size_t i = 0 ; i < key_size ; i++) {
        //printf("char %i: %d\n", i, table->array[pos*key_size+i]);
        if (table->array[pos*key_size + i] != 0) {
            return false;
        }
    }

    return true;
}

static inline bool cmpkey(table* table, char str[static 1], size_t pos, size_t key_size)
{
    for (size_t i = 0 ; i < key_size ; i++)
        if (table->array[pos*key_size+i] != str[i])
            return false;

    return true;
}

static inline void insert_key(table* table, char str[static 1], size_t pos, size_t key_size)
{
    for (size_t i = 0 ; i < key_size ; i++)
        table->array[pos*key_size+i] = str[i];
}


static inline void delete_key(table* table, size_t pos, size_t key_size)
{
    for (size_t i = 0 ; i < key_size ; i++)
        table->array[pos*key_size+i] = (char)255;
}

static inline size_t find_key(open_addressing_storage* _self, char str[static 1])
{
    size_t h = hash(str, _self->base.key_size);
    size_t pos = h & (_self->table->size - 1);

    if (cmpkey(_self->table, str, pos, _self->base.key_size))
        return pos;

    while (!is_empty(_self->table, pos, _self->base.key_size)) {
        h = rehash(h, str, _self->base.key_size);
        pos = h & (_self->table->size - 1);
        if (cmpkey(_self->table, str, pos, _self->base.key_size))
            return pos;
    }

    return pos;
}

static bool add(open_addressing_storage* _self, char str[static 1])
{
    if (_self->base.add != (add_storage_method)add) {
        _self->base.error = "wrong storage object";
        return false;
    }

    size_t h = hash(str, _self->base.key_size);
    size_t pos = h & (_self->table->size - 1);
    size_t available_pos = SIZE_MAX;

    //printf("pos:%d\thash:%d\n", pos, h);
    if (cmpkey(_self->table, str, pos, _self->base.key_size))
        return false;

    if (is_available(_self->table, pos, _self->base.key_size))
        available_pos = pos;

    while (!is_empty(_self->table, pos, _self->base.key_size)) {
        h = rehash(h, str, _self->base.key_size);
        pos = h & (_self->table->size - 1);
        //printf("pos:%d\thash:%d\n", pos, h);
        if (cmpkey(_self->table, str, pos, _self->base.key_size))
            return false;

        if (available_pos == SIZE_MAX && is_available(_self->table, pos, _self->base.key_size))
            available_pos = pos;
    }

    if (available_pos != SIZE_MAX)
        pos = available_pos;

    insert_key(_self->table, str, pos, _self->base.key_size);
    _self->base.count++;


    //printf("\nadded\n");
    return true;
}


static bool del(open_addressing_storage* _self, char str[static 1])
{
    if (_self->base.del != (del_storage_method)del) {
        _self->base.error = "wrong storage object";
        return false;
    }

    size_t pos = find_key(_self, str);

    if (cmpkey(_self->table, str, pos, _self->base.key_size)) {
        delete_key(_self->table, pos, _self->base.key_size);
        _self->base.count--;
        //printf("deleted\n");
        return true;
    }

    return false;
}

static bool has(open_addressing_storage* _self, char str[static 1])
{
    if (_self->base.has != (has_storage_method)has) {
        _self->base.error = "wrong storage object";
        return false;
    }

    size_t pos = find_key(_self, str);

//    printf("%.*s <=> ", _self->base.key_size, str);
//    for (size_t i=0 ; i < _self->base.key_size ; i++)
//        printf("%c", _self->table->array[pos*_self->base.key_size+i]);
//
//    printf("\n");

    if (cmpkey(_self->table, str, pos, _self->base.key_size))
        return true;

    return false;
}

static bool free_storage(open_addressing_storage* _self)
{
    if (_self->base.free_storage != (free_storage_storage_method)free_storage) {
        _self->base.error = "wrong storage object";
        return false;
    }

    if (_self->base.error) {
        efree(_self->base.error);
        _self->base.error = NULL;
    }

    efree(_self->table);
    _self->table = NULL;
}

