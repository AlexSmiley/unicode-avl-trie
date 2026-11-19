/*
 * Unicode-aware AVL Trie dictionary.
 *
 * Each AVL tree contains alternative Unicode code points at one word position.
 * `next` points to the AVL tree for the following code point.
 *
 * Supports:
 *   - lowercase and uppercase letters
 *   - digits and punctuation
 *   - valid UTF-8 words (including Cyrillic, e.g. Macedonian)
 *   - UTF-8 translations
 *
 * Compile:
 *   gcc -std=c11 -O2 -Wall -Wextra -pedantic unicode_avl_trie.c -o unicode_avl_trie
 *
 * Note: UTF-8 is decoded into Unicode code points. This does not perform
 * Unicode normalization or locale-specific case folding.
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_TRANSLATION 512

typedef struct Dictionary {
    uint32_t letter;                 /* One Unicode code point */
    char translation[MAX_TRANSLATION];
    int height;
    struct Dictionary *left;         /* Smaller alternative at this level */
    struct Dictionary *right;        /* Larger alternative at this level */
    struct Dictionary *next;         /* AVL tree for the next character */
} Dictionary;

static int height(const Dictionary *node)
{
    return node ? node->height : 0;
}

static int max_int(int a, int b)
{
    return a > b ? a : b;
}

static void update_height(Dictionary *node)
{
    if (node)
        node->height = 1 + max_int(height(node->left), height(node->right));
}

static int balance_factor(const Dictionary *node)
{
    return node ? height(node->left) - height(node->right) : 0;
}

static Dictionary *rotate_right(Dictionary *y)
{
    Dictionary *x = y->left;
    Dictionary *middle = x->right;

    x->right = y;
    y->left = middle;

    update_height(y);
    update_height(x);
    return x;
}

static Dictionary *rotate_left(Dictionary *x)
{
    Dictionary *y = x->right;
    Dictionary *middle = y->left;

    y->left = x;
    x->right = middle;

    update_height(x);
    update_height(y);
    return y;
}

static Dictionary *rebalance(Dictionary *node)
{
    int balance;

    if (!node)
        return NULL;

    update_height(node);
    balance = balance_factor(node);

    if (balance > 1) {
        if (balance_factor(node->left) < 0)
            node->left = rotate_left(node->left);
        return rotate_right(node);
    }

    if (balance < -1) {
        if (balance_factor(node->right) > 0)
            node->right = rotate_right(node->right);
        return rotate_left(node);
    }

    return node;
}

static Dictionary *create_node(uint32_t code_point)
{
    Dictionary *node = (Dictionary *)calloc(1, sizeof(*node));

    if (!node) {
        perror("calloc");
        exit(EXIT_FAILURE);
    }

    node->letter = code_point;
    node->height = 1;
    return node;
}

static Dictionary *find_node(Dictionary *root, uint32_t code_point)
{
    while (root) {
        if (code_point < root->letter)
            root = root->left;
        else if (code_point > root->letter)
            root = root->right;
        else
            return root;
    }

    return NULL;
}

static Dictionary *insert_node(Dictionary *root,
                               uint32_t code_point,
                               Dictionary **found)
{
    if (!root) {
        *found = create_node(code_point);
        return *found;
    }

    if (code_point < root->letter)
        root->left = insert_node(root->left, code_point, found);
    else if (code_point > root->letter)
        root->right = insert_node(root->right, code_point, found);
    else {
        *found = root;
        return root;
    }

    return rebalance(root);
}

/* Remove the smallest AVL node and return the new subtree root.
 * The removed node is returned through `minimum`.
 */
static Dictionary *detach_min(Dictionary *root, Dictionary **minimum)
{
    if (!root->left) {
        Dictionary *right = root->right;
        root->right = NULL;
        *minimum = root;
        return right;
    }

    root->left = detach_min(root->left, minimum);
    return rebalance(root);
}

static Dictionary *delete_node(Dictionary *root, uint32_t code_point)
{
    if (!root)
        return NULL;

    if (code_point < root->letter) {
        root->left = delete_node(root->left, code_point);
    } else if (code_point > root->letter) {
        root->right = delete_node(root->right, code_point);
    } else {
        if (!root->left || !root->right) {
            Dictionary *replacement = root->left ? root->left : root->right;
            free(root);
            return replacement;
        } else {
            Dictionary *successor = NULL;
            Dictionary *right_subtree = detach_min(root->right, &successor);

            /* Move the successor's complete word payload into this AVL node. */
            root->letter = successor->letter;
            memcpy(root->translation, successor->translation,
                   sizeof(root->translation));
            root->next = successor->next;

            successor->next = NULL;
            free(successor);

            root->right = right_subtree;
        }
    }

    return rebalance(root);
}

/* Decode exactly one UTF-8 code point.
 * Returns 1 on success, 0 for invalid or incomplete UTF-8.
 * Advances *cursor only on success.
 */
static int utf8_next(const unsigned char **cursor, uint32_t *code_point)
{
    const unsigned char *s = *cursor;
    uint32_t cp;
    size_t count;

    if (s[0] == 0)
        return 0;

    if (s[0] <= 0x7F) {
        cp = s[0];
        count = 1;
    } else if (s[0] >= 0xC2 && s[0] <= 0xDF) {
        cp = s[0] & 0x1F;
        count = 2;
    } else if (s[0] >= 0xE0 && s[0] <= 0xEF) {
        cp = s[0] & 0x0F;
        count = 3;
    } else if (s[0] >= 0xF0 && s[0] <= 0xF4) {
        cp = s[0] & 0x07;
        count = 4;
    } else {
        return 0;
    }

    for (size_t i = 1; i < count; ++i) {
        if (s[i] == 0 || (s[i] & 0xC0) != 0x80)
            return 0;
        cp = (cp << 6) | (s[i] & 0x3F);
    }

    /* Reject overlong encodings, UTF-16 surrogates and out-of-range values. */
    if ((count == 2 && cp < 0x80) ||
        (count == 3 && cp < 0x800) ||
        (count == 4 && cp < 0x10000) ||
        (cp >= 0xD800 && cp <= 0xDFFF) ||
        cp > 0x10FFFF)
        return 0;

    *cursor = s + count;
    *code_point = cp;
    return 1;
}

static int decode_word(const char *word, uint32_t **out, size_t *out_length)
{
    const unsigned char *cursor;
    uint32_t *points;
    size_t capacity, length = 0;

    if (!word || !word[0])
        return 0;

    capacity = strlen(word); /* At most one code point per UTF-8 byte. */
    points = (uint32_t *)malloc(capacity * sizeof(*points));
    if (!points) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    cursor = (const unsigned char *)word;
    while (*cursor) {
        uint32_t cp;
        if (!utf8_next(&cursor, &cp)) {
            free(points);
            return 0;
        }
        points[length++] = cp;
    }

    *out = points;
    *out_length = length;
    return length != 0;
}

/* Returns 1 on success, 0 for invalid input or an overlong translation.
 * Adding an existing word updates its translation.
 */
int dictionary_add(Dictionary **root, const char *word, const char *translation)
{
    uint32_t *points = NULL;
    size_t length = 0;

    if (!root || !translation ||
        strlen(translation) >= MAX_TRANSLATION ||
        !decode_word(word, &points, &length))
        return 0;

    Dictionary **level = root;

    for (size_t i = 0; i < length; ++i) {
        Dictionary *node = NULL;
        *level = insert_node(*level, points[i], &node);

        if (i + 1 == length) {
            memcpy(node->translation, translation, strlen(translation) + 1);
        } else {
            level = &node->next;
        }
    }

    free(points);
    return 1;
}

/* Returned translation belongs to the dictionary; do not free or modify it. */
const char *dictionary_search(Dictionary *root, const char *word)
{
    uint32_t *points = NULL;
    size_t length = 0;
    Dictionary *level = root;
    const char *result = NULL;

    if (!decode_word(word, &points, &length))
        return NULL;

    for (size_t i = 0; i < length; ++i) {
        Dictionary *node = find_node(level, points[i]);
        if (!node)
            goto done;

        if (i + 1 == length) {
            if (node->translation[0])
                result = node->translation;
        } else {
            level = node->next;
        }
    }

done:
    free(points);
    return result;
}

static int remove_recursive(Dictionary **level,
                            const uint32_t *points,
                            size_t index,
                            size_t length)
{
    Dictionary *node = find_node(*level, points[index]);

    if (!node)
        return 0;

    if (index + 1 < length) {
        if (!remove_recursive(&node->next, points, index + 1, length))
            return 0;
    } else {
        if (!node->translation[0])
            return 0;
        node->translation[0] = '\0';
    }

    /* Keep nodes that are word endings or still have following characters. */
    if (!node->translation[0] && !node->next)
        *level = delete_node(*level, points[index]);

    return 1;
}

int dictionary_remove(Dictionary **root, const char *word)
{
    uint32_t *points = NULL;
    size_t length = 0;
    int removed;

    if (!root || !decode_word(word, &points, &length))
        return 0;

    removed = remove_recursive(root, points, 0, length);
    free(points);
    return removed;
}

static void free_level(Dictionary *root)
{
    if (!root)
        return;

    free_level(root->left);
    free_level(root->right);
    free_level(root->next);
    free(root);
}

void dictionary_free(Dictionary *root)
{
    free_level(root);
}

int main(void)
{
    Dictionary *dictionary = NULL;
    const char *translation;

    /* English: uppercase, lowercase and digits. */
    dictionary_add(&dictionary, "Map42", "A map with numeric suffix");
    dictionary_add(&dictionary, "map", "карта");
    dictionary_add(&dictionary, "Apple", "јаболко");
    dictionary_add(&dictionary, "A1", "Letter A followed by digit one");

    /* Macedonian Cyrillic UTF-8 examples. */
    dictionary_add(&dictionary, "мачка", "cat");
    dictionary_add(&dictionary, "Македонија", "North Macedonia");
    dictionary_add(&dictionary, "2026", "year");

    const char *tests[] = {
        "Map42", "map", "Apple", "A1",
        "мачка", "Македонија", "2026", "missing"
    };

    puts("Search tests:");
    for (size_t i = 0; i < sizeof(tests) / sizeof(tests[0]); ++i) {
        translation = dictionary_search(dictionary, tests[i]);
        printf("  %-20s -> %s\n",
               tests[i], translation ? translation : "NOT FOUND");
    }

    puts("\nRemoval test:");
    printf("  Remove 'мачка': %s\n",
           dictionary_remove(&dictionary, "мачка") ? "OK" : "NOT FOUND");
    printf("  Search 'мачка': %s\n",
           dictionary_search(dictionary, "мачка") ?
           "unexpectedly found" : "NOT FOUND (expected)");

    dictionary_free(dictionary);
    return 0;
}