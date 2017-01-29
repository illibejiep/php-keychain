#ifndef PHP_KEYCHAIN_H
#define PHP_KEYCHAIN_H 1

#include "php.h"
#include "zend_exceptions.h"
#include "storage/storage.h"

#define PHP_KEYCHAIN_VERSION "0.1"
#define PHP_KEYCHAIN_EXTNAME "keychain"

#define SORT_ARRAY_TYPE 1
#define HASH_CHAINING_TYPE 2
#define OPEN_ADDRESSING_TYPE 3

PHP_MINIT_FUNCTION(keychain);

PHP_METHOD(Keychain, __construct);
PHP_METHOD(Keychain, add);
PHP_METHOD(Keychain, del);
PHP_METHOD(Keychain, has);

typedef struct keychain_zend_object keychain_zend_object;

struct keychain_zend_object {
    keys_storage* storage;
    zend_object zobj;
};

static zend_object* keychain_create_object(zend_class_entry *ce);
void keychain_destroy_object(zend_object *object);
void keychain_free_object(zend_object *object);


static inline keychain_zend_object* php_custom_object_fetch_object(zend_object *obj) {
    return (keychain_zend_object*)((char *)obj - XtOffsetOf(keychain_zend_object, zobj));
}

#define Z_CUSTOM_OBJ_P(zv) php_custom_object_fetch_object(Z_OBJ_P(zv));

extern zend_module_entry keychain_module_entry;
#define phpext_keychain_ptr &keychain_module_entry

#endif