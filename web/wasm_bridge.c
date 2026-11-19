#include <emscripten/emscripten.h>
#include "../unicode_avl_trie.c"

static Dictionary *dictionary_root = NULL;

EMSCRIPTEN_KEEPALIVE void wasm_init(void) {
    dictionary_free(dictionary_root);
    dictionary_root = NULL;
}
EMSCRIPTEN_KEEPALIVE int wasm_add(const char *word, const char *translation) {
    return dictionary_add(&dictionary_root, word, translation);
}
EMSCRIPTEN_KEEPALIVE const char *wasm_search(const char *word) {
    return dictionary_search(dictionary_root, word);
}
EMSCRIPTEN_KEEPALIVE int wasm_remove(const char *word) {
    return dictionary_remove(&dictionary_root, word);
}
EMSCRIPTEN_KEEPALIVE void wasm_clear(void) {
    dictionary_free(dictionary_root);
    dictionary_root = NULL;
}
