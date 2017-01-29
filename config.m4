PHP_ARG_ENABLE(keychain, whether to enable Keychain extension,
[ --enable-keychain   Enable Keychain extension])

if test "$PHP_KEYCHAIN" = "yes"; then
  AC_DEFINE(HAVE_KEYCHAIN, 1, [Whether you have Keychain])
  PHP_NEW_EXTENSION(keychain, keychain.c storage/storage.c storage/sort_array_storage.c storage/hash_chaining_storage.c storage/open_addressing_storage.c, $ext_shared)
fi