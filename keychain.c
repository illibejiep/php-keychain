#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "keychain.h"
#include "storage/sort_array_storage.h"
#include "storage/hash_chaining_storage.h"
#include "storage/open_addressing_storage.h"

zend_module_entry keychain_module_entry = {
#if ZEND_MODULE_API_NO >= 20010901
    STANDARD_MODULE_HEADER,
#endif
    PHP_KEYCHAIN_EXTNAME,
    NULL,
    PHP_MINIT(keychain),
    NULL,
    NULL,
    NULL,
    NULL,
#if ZEND_MODULE_API_NO >= 20010901
    PHP_KEYCHAIN_VERSION,
#endif
    STANDARD_MODULE_PROPERTIES
};

#ifdef COMPILE_DL_KEYCHAIN
ZEND_GET_MODULE(keychain)
#endif


static zend_function_entry keychain_methods[] = {
        PHP_ME(Keychain, __construct, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)
        PHP_ME(Keychain, add, NULL, ZEND_ACC_PUBLIC)
        PHP_ME(Keychain, del, NULL, ZEND_ACC_PUBLIC)
        PHP_ME(Keychain, has, NULL, ZEND_ACC_PUBLIC)
        {NULL, NULL, NULL}
};


zend_class_entry *keychain_ce;
zend_class_entry *keychain_exception_ce;
zend_object_handlers keychain_class_handlers;

void keychain_init(TSRMLS_D) {
    //printf("init class\n");
    zend_class_entry ce;

    INIT_CLASS_ENTRY(ce, "Keychain", keychain_methods);
    keychain_ce = zend_register_internal_class(&ce TSRMLS_CC);

    keychain_ce->create_object = keychain_create_object; /* our create handler */

    memcpy(&keychain_class_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));

    keychain_class_handlers.free_obj = keychain_free_object; /* This is the free handler */
    keychain_class_handlers.dtor_obj = keychain_destroy_object; /* This is the dtor handler */
    /* my_ce_handlers.clone_obj is also available though we won't use it here */
    keychain_class_handlers.offset   = XtOffsetOf(keychain_zend_object, zobj); /* Here, we declare the offset to the engine */

    //printf("init class fields\n");
    /* fields */
    zend_declare_property_long(keychain_ce, "count", strlen("count"), 0, ZEND_ACC_PROTECTED);
    zend_declare_property_long(keychain_ce, "size", strlen("size"), 0, ZEND_ACC_PROTECTED);
    zend_declare_property_long(keychain_ce, "allocated", strlen("allocated"), 0, ZEND_ACC_PROTECTED);

    zend_declare_class_constant_long(keychain_ce, "SORT_ARRAY_TYPE", strlen("SORT_ARRAY_TYPE"), SORT_ARRAY_TYPE);
    zend_declare_class_constant_long(keychain_ce, "HASH_CHAINING_TYPE", strlen("HASH_CHAINING_TYPE"), HASH_CHAINING_TYPE);
    zend_declare_class_constant_long(keychain_ce, "OPEN_ADDRESSING_TYPE", strlen("OPEN_ADDRESSING_TYPE"), OPEN_ADDRESSING_TYPE);

    zend_class_entry exception_ce;
    INIT_CLASS_ENTRY(exception_ce, "KeychainException", NULL);
    keychain_exception_ce = zend_register_internal_class_ex(&exception_ce, zend_ce_exception);
}

static zend_object* keychain_create_object(zend_class_entry *ce TSRMLS_DC)
{
    //printf("create class\n");
    keychain_zend_object *obj;

    obj = ecalloc(1, sizeof(keychain_zend_object) + zend_object_properties_size(ce));
    obj->storage = NULL;

    zend_object_std_init(&(obj->zobj), ce TSRMLS_CC);
    object_properties_init(&(obj->zobj), ce);

    obj->zobj.handlers = &keychain_class_handlers;

    //printf("created class\n");

    return &(obj->zobj);
}

void keychain_destroy_object(zend_object *object)
{
    //printf("destroy\n");

    zend_objects_destroy_object(object);
    //printf("destroy\n");
}

void keychain_free_object(zend_object *object)
{
    //printf("free\n");
    keychain_zend_object* keychain_obj = php_custom_object_fetch_object(object);

    if (keychain_obj->storage) {
        keychain_obj->storage->free_storage(keychain_obj->storage);
        efree(keychain_obj->storage);
    }
    efree(keychain_obj);
    //printf("free\n");

}


PHP_MINIT_FUNCTION(keychain) {
    keychain_init(TSRMLS_C);
}

PHP_METHOD(Keychain, __construct) {
    //printf("__constructor\n");
    size_t type;
    size_t size;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l|l", &size, &type) == FAILURE) {
        return;
    }

    keys_storage* storage = NULL;

    switch (type) {
        case SORT_ARRAY_TYPE:
            storage = sort_array_storage_create(size);
            break;
        case OPEN_ADDRESSING_TYPE:
            storage = open_addressing_storage_create(size);
            break;
        case HASH_CHAINING_TYPE:
            storage = hash_chaining_storage_create(size);
            break;
        default:
            zend_throw_exception(keychain_exception_ce, "unknown storage type", 1);
            return;
    }

    //printf("created %p\n", storage);

    keychain_zend_object* keychain_obj = php_custom_object_fetch_object(getThis()->value.obj);
    keychain_obj->storage = storage;

    zend_update_property_long(keychain_ce, getThis(), "size", strlen("size"), size TSRMLS_CC);
    zend_update_property_long(keychain_ce, getThis(), "count", strlen("count"), storage->count TSRMLS_CC);
    zend_update_property_long(keychain_ce, getThis(), "allocated", strlen("allocated"), storage->allocated TSRMLS_CC);

    //printf("created %p\n", storage);
}

PHP_METHOD(Keychain, add) {

    HashTable* arr;
    ALLOC_HASHTABLE(arr);
    ZEND_INIT_SYMTABLE_EX(arr, 32, 0);

    zend_hash_copy(arr, EG(function_table), NULL);

    RETURN_ARR(arr);

    char* str;
    size_t len;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &str, &len) == FAILURE) {
        return;
    }

    keychain_zend_object* keychain_obj = php_custom_object_fetch_object(getThis()->value.obj);
    keys_storage* storage = keychain_obj->storage;

    //printf("storage %p\n", storage);

    char* key = emalloc(storage->key_size);
    //printf("%p\n", key);
    memset(key, 0, storage->key_size);
    memcpy(key, str, len);

    bool result = storage->add(storage, key);

    efree(key);

    zend_update_property_long(keychain_ce, getThis(), "count", strlen("count"), storage->count TSRMLS_CC);
    zend_update_property_long(keychain_ce, getThis(), "allocated", strlen("allocated"), storage->allocated TSRMLS_CC);

    RETURN_BOOL(result);
}

PHP_METHOD(Keychain, del) {
    char* str;
    size_t len;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &str, &len) == FAILURE) {
        return;
    }

    keychain_zend_object* keychain_obj = php_custom_object_fetch_object(getThis()->value.obj);
    keys_storage* storage = keychain_obj->storage;

    char* key = emalloc(storage->key_size);
    memset(key, 0, storage->key_size);
    memcpy(key, str, len);

    bool result = storage->del(storage, key);

    efree(key);

    zend_update_property_long(keychain_ce, getThis(), "count", strlen("count"), storage->count TSRMLS_CC);
    zend_update_property_long(keychain_ce, getThis(), "allocated", strlen("allocated"), storage->allocated TSRMLS_CC);

    RETURN_BOOL(result);
}

ZEND_METHOD(Keychain, has) {
    char* str;
    size_t len;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &str, &len) == FAILURE) {
        return;
    }

    keychain_zend_object* keychain_obj = php_custom_object_fetch_object(getThis()->value.obj);
    keys_storage* storage = keychain_obj->storage;

    char* key = emalloc(storage->key_size);
    memset(key, 0, storage->key_size);
    memcpy(key, str, len);

    bool result = storage->has(storage, key);

    efree(key);

    RETURN_BOOL(result);
}