# Unicode AVL Trie Dictionary (C)

A dictionary implemented as a Trie whose sibling alternatives at each character position are stored in an AVL tree.

## Design

- Each node stores one Unicode code point (`uint32_t`).
- `left` and `right` form an AVL tree of alternative characters at the same word position.
- `next` points to the AVL tree for the next character in the word.
- A non-empty `translation` marks the end of a stored word. A node can be both a complete word and a prefix of a longer word.

This gives balanced lookup among alternative characters at each Trie level while retaining shared prefixes.

## Features

- Uppercase and lowercase letters (case-sensitive).
- Digits and punctuation.
- Valid UTF-8 input, including Cyrillic text such as Macedonian.
- UTF-8 translations.
- Add/update, search, remove, and free operations.
- Rejects malformed UTF-8, overlong encodings, surrogate code points, and values above U+10FFFF.

The implementation compares Unicode code points numerically. It does **not** perform Unicode normalization or locale-specific case folding/collation.

## Build

Requires a C11-compatible compiler:

```sh
gcc -std=c11 -O2 -Wall -Wextra -pedantic unicode_avl_trie.c -o unicode_avl_trie
```

## Run

```sh
./unicode_avl_trie
```

The included `main` demonstrates English words, mixed-case input, digits, Macedonian Cyrillic, searching, and removal.

## API

```c
int dictionary_add(Dictionary **root, const char *word, const char *translation);
const char *dictionary_search(Dictionary *root, const char *word);
int dictionary_remove(Dictionary **root, const char *word);
void dictionary_free(Dictionary *root);
```

- `dictionary_add` returns 1 on success, 0 for invalid input or a translation that exceeds the fixed buffer. Adding an existing word updates its translation.
- `dictionary_search` returns a pointer to the stored translation, or `NULL` if the word is absent. The returned pointer is owned by the dictionary.
- `dictionary_remove` returns 1 if a stored word was removed, otherwise 0.
- Call `dictionary_free` when finished.

## Translation limit

Translations are stored in a fixed-size buffer of 512 bytes per terminal-capable node, including the terminating NUL byte. The maximum accepted UTF-8 translation length is therefore 511 bytes.
