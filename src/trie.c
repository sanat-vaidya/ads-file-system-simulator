#include "trie.h"
#include <stdbool.h>
#include <stdlib.h>

static TrieNode *root = NULL;

static TrieNode *new_node(void) {
    TrieNode *t = malloc(sizeof(TrieNode));
    for (int i = 0; i < ALPHABET_SIZE; i++) t->child[i] = NULL;
    t->ids = NULL;
    return t;
}

static void free_node(TrieNode *t) {
    for (int i = 0; i < ALPHABET_SIZE; i++) {
        if (t->child[i] != NULL) free_node(t->child[i]);
    }
    IdNode *p = t->ids;
    while (p != NULL) {
        IdNode *next = p->next;
        free(p);
        p = next;
    }
    free(t);
}

void trie_init(void) {
    root = new_node();
}

void trie_free_all(void) {
    free_node(root);
    root = NULL;
}

// position of c in ALPHABET, or -1 if c is not allowed
static int index_of(char c) {
    for (int i = 0; i < ALPHABET_SIZE; i++) {
        if (ALPHABET[i] == c) return i;
    }
    return -1;
}

void trie_insert(const char *name, int id) {
    TrieNode *t = root;
    for (int i = 0; name[i] != '\0'; i++) {
        int c = index_of(name[i]);
        if (c == -1) return;
        if (t->child[c] == NULL) t->child[c] = new_node();
        t = t->child[c];
    }

    // add the id at the end of the list, so results keep creation order
    IdNode *node = malloc(sizeof(IdNode));
    node->id = id;
    node->next = NULL;
    if (t->ids == NULL) {
        t->ids = node;
    } else {
        IdNode *last = t->ids;
        while (last->next != NULL) last = last->next;
        last->next = node;
    }
}

// name is what is left of the name below t.
// Returns true if t is now empty (no ids, no children), so its parent can free it.
static bool remove_from(TrieNode *t, const char *name, int id) {
    if (name[0] == '\0') {
        // unlink id from the list
        IdNode *p = t->ids;
        IdNode *prev = NULL;
        while (p != NULL && p->id != id) {
            prev = p;
            p = p->next;
        }
        if (p != NULL) {
            if (prev == NULL) t->ids = p->next;
            else prev->next = p->next;
            free(p);
        }
    } else {
        int c = index_of(name[0]);
        if (c == -1 || t->child[c] == NULL) return false;
        if (remove_from(t->child[c], name + 1, id)) {
            free(t->child[c]);
            t->child[c] = NULL;
        }
    }

    if (t->ids != NULL) return false;
    for (int i = 0; i < ALPHABET_SIZE; i++) {
        if (t->child[i] != NULL) return false;
    }
    return true;
}

void trie_remove(const char *name, int id) {
    remove_from(root, name, id);
}

// DFS below t: shorter names first, then children in alphabet order
static void collect(TrieNode *t, int out[], int *n) {
    for (IdNode *p = t->ids; p != NULL; p = p->next) {
        out[*n] = p->id;
        (*n)++;
    }
    for (int i = 0; i < ALPHABET_SIZE; i++) {
        if (t->child[i] != NULL) collect(t->child[i], out, n);
    }
}

int trie_search(const char *prefix, int out[]) {
    TrieNode *t = root;
    for (int i = 0; prefix[i] != '\0'; i++) {
        int c = index_of(prefix[i]);
        if (c == -1 || t->child[c] == NULL) return 0;
        t = t->child[c];
    }
    int n = 0;
    collect(t, out, &n);
    return n;
}
