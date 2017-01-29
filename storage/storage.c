
#include "storage.h"

size_t keys_storage_errno = 0;
char* keys_storage_errors[3] = {
        [0]                             = "Ok",
        [WRONG_STORAGE_OBJECT_ERRNO]    = "Wrong storage object",
        [WRONG_KEY_ERRNO]               = "Wrong key string",
};
