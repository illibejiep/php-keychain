//
// Created by illibejiep on 12/27/16.
//

#ifndef KEYCHAIN_STORAGE_H
#define KEYCHAIN_STORAGE_H

#include <stdbool.h>
#include <stdlib.h>
#include "php.h"

typedef struct keys_storage keys_storage;
typedef bool (*add_storage_method)(keys_storage* _self, char str[static 1]);
typedef bool (*del_storage_method)(keys_storage* _self, char str[static 1]);
typedef bool (*has_storage_method)(keys_storage* _self, char str[static 1]);
typedef bool (*free_storage_storage_method )(keys_storage* _self);

struct keys_storage {
    size_t count;
    size_t key_size;
    size_t allocated;
    float alfa;
    char* error;
    add_storage_method add;
    del_storage_method del;
    has_storage_method has;
    free_storage_storage_method free_storage;
};

#endif //KEYCHAIN_STORAGE_H
