//
// Created by illibejiep on 12/27/16.
//

#include "hash_chaining_storage.h"

#define INIT_NSIZE 1024

typedef struct _backet backet;
struct _backet {
    backet* next;
    char key[1];
};

typedef struct _table table;
struct _table {
    size_t size;
    backet* backets[INIT_NSIZE];
};

typedef struct _hash_chaining_storage hash_chaining_storage;
struct _hash_chaining_storage {
    keys_storage base;
    table* table;
};

static inline backet* create_bucket(hash_chaining_storage* _self, char str[static 1]);
static inline size_t get_pos(hash_chaining_storage* _self, char str[static 1]);
static void table_doubling(hash_chaining_storage* _self);
static void table_halving(hash_chaining_storage* _self);

static bool add(hash_chaining_storage* _self, char str[static 1]);
static bool del(hash_chaining_storage* _self, char str[static 1]);
static bool has(hash_chaining_storage* _self, char str[static 1]);
static bool free_storage(hash_chaining_storage* _self);


keys_storage * hash_chaining_storage_create(size_t key_size)
{

    hash_chaining_storage* storage = emalloc(sizeof(hash_chaining_storage));

    storage->base.count = 0;
    storage->base.key_size = key_size;

    storage->base.allocated = sizeof(table);

    storage->table = emalloc(sizeof(table));
    memset(storage->table, 0, sizeof(table));
    storage->table->size = INIT_NSIZE;

    storage->base.add = (add_storage_method)add;
    storage->base.del = (del_storage_method)del;
    storage->base.has = (has_storage_method)has;
    storage->base.free_storage = (free_storage_storage_method)free_storage;

    return (keys_storage*)storage;
}

static inline backet* create_bucket(hash_chaining_storage* _self, char str[static 1])
{
    backet* newBacket = emalloc(sizeof(backet) + _self->base.key_size);
    newBacket->next = NULL;
    memcpy(newBacket->key, str, _self->base.key_size);

    return newBacket;
}

static inline size_t get_pos(hash_chaining_storage* _self, char str[static 1])
{
    size_t hash = 5381;
    for (size_t i=0; i < _self->base.key_size ; i++) {
        hash = ((hash << 5) + hash) + str[i]; /* hash * 33 + c */
    }

    //printf("pos:%d\n", hash & (table->size-1));

    return hash & (_self->table->size - 1);
}

static void table_doubling(hash_chaining_storage* _self)
{
    printf("doubling from %d...\n", _self->table->size);

    _self->table->size *= 2;
    _self->table = erealloc(_self->table, sizeof(table) + (_self->table->size - INIT_NSIZE)*sizeof(backet*));

    for (size_t i = _self->table->size/2 ; i < _self->table->size ; i++) {
        _self->table->backets[i] = NULL;
    }

    for (size_t i = 0 ; i < _self->table->size/2 ; i++) {
        if (!_self->table->backets[i])
            continue;

        backet* currentBucket = _self->table->backets[i];
        backet* prevBucket = NULL;
        while (currentBucket) {
            size_t pos = get_pos(_self, currentBucket->key);
            //printf("pos:%d i:%d key:%.*s\n", pos, i, storage->key_size, currentBucket->key);
            if (pos == i) {
                prevBucket = currentBucket;
                currentBucket = currentBucket->next;
                continue;
            }

            // move backet
            backet* toInsertBucket = _self->table->backets[pos];
            if (toInsertBucket) {
                while (toInsertBucket->next) {
                    toInsertBucket = toInsertBucket->next;
                }
                toInsertBucket->next = currentBucket;
            } else {
                _self->table->backets[pos] = currentBucket;
            }

            if (prevBucket) {
                prevBucket->next = currentBucket->next;
            } else {
                _self->table->backets[i] = currentBucket->next;
            }

            currentBucket = currentBucket->next;
            if (toInsertBucket) {
                toInsertBucket->next->next = NULL;
            } else {
                _self->table->backets[pos]->next = NULL;
            }
        }

    }

//    for (size_t i=0 ; i < table->size ; i++) {
//        if (!table->backets[i])
//            continue;
//
//
//        printf("%d: %p", i, table->backets[i]);
//        backet* currentBacket = table->backets[i];
//        while (currentBacket->next) {
//            printf(" -> %p", currentBacket->next);
//            currentBacket = currentBacket->next;
//        }
//        printf("\n");
//    }
    printf("doubled to %d\n", _self->table->size);
}

static void table_halving(hash_chaining_storage* _self)
{
    printf("halving from %d...\n", _self->table->size);
    _self->table->size /= 2;

    for (size_t i = _self->table->size ; i < _self->table->size*2 ; i++) {
        if (_self->table->backets[i] == NULL)
            continue;

        size_t pos = get_pos(_self, _self->table->backets[i]->key);
        backet* toInsertBucket = _self->table->backets[pos];
        if (toInsertBucket) {
            while (toInsertBucket->next) {
                toInsertBucket = toInsertBucket->next;
            }
            toInsertBucket->next = _self->table->backets[i];
        } else {
            _self->table->backets[pos] = _self->table->backets[i];
        }
    }

    _self->table = erealloc(_self->table, sizeof(table) + (_self->table->size - INIT_NSIZE)*sizeof(backet*));

    printf("halved to %d\n", _self->table->size);
}

static bool add(hash_chaining_storage* _self, char str[static 1])
{
    size_t pos = get_pos(_self, str);
    //printf("add pos:%d key:%.*s\n", pos, storage->key_size, str);
    if (!_self->table->backets[pos]) {
        _self->table->backets[pos] = create_bucket(_self, str);
    } else {
        backet* currentBacket = _self->table->backets[pos];
//        printf("table: %p\n", table->backets[pos]);
//        printf("backet %d: %p\n", pos, currentBacket);
//        printf("next: %p\n", currentBacket->next);
        while (currentBacket->next) {
            if (memcmp(currentBacket->key, str, _self->base.key_size) == 0) {
                return false;
            }
            currentBacket = currentBacket->next;
        }

        currentBacket->next = create_bucket(_self, str);
    }

    _self->base.count++;
    _self->base.allocated += sizeof(backet) + _self->base.key_size;

    // doubling
    if (_self->table->size <= _self->base.count) {
        table_doubling(_self);
    }

    return true;
}

static bool del(hash_chaining_storage* _self, char str[static 1])
{
    size_t pos = get_pos(_self, str);

    if (!_self->table->backets[pos])
        return false;

    backet* currentBacket = _self->table->backets[pos];
    backet* prevBacket = NULL;

    while (currentBacket) {
        if (memcmp(currentBacket->key, str, _self->base.key_size) == 0) {
            if (prevBacket) {
                prevBacket->next = currentBacket->next;
            } else {
                _self->table->backets[pos] = currentBacket->next;
            }
            efree(currentBacket);

            _self->base.count--;
            _self->base.allocated -= sizeof(backet) + _self->base.key_size;

            // halving
            if (_self->table->size > INIT_NSIZE && _self->table->size/4 >= _self->base.count) {
                printf("table->size=%d storage->count=%d\n", _self->table->size, _self->base.count);
                table_halving(_self);
            }

            return true;
        }
        prevBacket = currentBacket;
        currentBacket = currentBacket->next;
    }

    return false;
}

static bool has(hash_chaining_storage* _self, char str[static 1])
{
    size_t pos = get_pos(_self, str);

    if (!_self->table->backets[pos])
        return false;

    backet* currentBacket = _self->table->backets[pos];

    while (currentBacket) {
        if (memcmp(currentBacket->key, str, _self->base.key_size) == 0) {
            return true;
        }
        currentBacket = currentBacket->next;
    }

    return false;
}


static bool free_storage(hash_chaining_storage* _self)
{
    for (size_t i = 0 ; i < _self->table->size ; i++) {
        if (!_self->table->backets[i])
            continue;

        backet* currentBucket = _self->table->backets[i];
        backet* nextBacket = currentBucket->next;
        while(currentBucket) {
            efree(currentBucket);
            currentBucket = nextBacket;
            nextBacket = currentBucket ? currentBucket->next : NULL;
        }
    }

    efree(_self->table);
    _self->table = NULL;
}