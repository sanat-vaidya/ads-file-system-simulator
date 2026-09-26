#include "btree.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static BTreeNode *new_node(bool leaf) {
    BTreeNode *x = malloc(sizeof(BTreeNode));
    x->n = 0;
    x->leaf = leaf;
    for (int i = 0; i <= MAX_KEYS; i++) x->child[i] = NULL;
    return x;
}

static void free_node(BTreeNode *x) {
    if (!x->leaf) {
        for (int i = 0; i <= x->n; i++) free_node(x->child[i]);
    }
    free(x);
}

// copy key and value from src slot j into dst slot i
static void copy_key(BTreeNode *dst, int i, BTreeNode *src, int j) {
    strcpy(dst->keys[i], src->keys[j]);
    dst->vals[i] = src->vals[j];
}

BTree *btree_new(void) {
    BTree *t = malloc(sizeof(BTree));
    t->root = new_node(true);
    t->count = 0;
    return t;
}

void btree_free(BTree *t) {
    if (t == NULL) return;
    free_node(t->root);
    free(t);
}

int btree_size(BTree *t) {
    return t->count;
}

int btree_height(BTree *t) {
    int h = 1;
    BTreeNode *x = t->root;
    while (!x->leaf) {
        x = x->child[0];
        h++;
    }
    return h;
}

// ---------------- find ----------------

int btree_find(BTree *t, const char *key) {
    BTreeNode *x = t->root;
    while (true) {
        int i = 0;
        while (i < x->n && strcmp(key, x->keys[i]) > 0) i++;
        if (i < x->n && strcmp(key, x->keys[i]) == 0) return x->vals[i];
        if (x->leaf) return -1;
        x = x->child[i];
    }
}

// ---------------- list (in-order traversal) ----------------

static void list_from(BTreeNode *x, int vals[], int *n) {
    for (int i = 0; i < x->n; i++) {
        if (!x->leaf) list_from(x->child[i], vals, n);
        vals[*n] = x->vals[i];
        (*n)++;
    }
    if (!x->leaf) list_from(x->child[x->n], vals, n);
}

int btree_list(BTree *t, int vals[]) {
    int n = 0;
    list_from(t->root, vals, &n);
    return n;
}

// ---------------- insert ----------------

// x->child[i] is full. Split it in two and move its middle key up into x.
static void split_child(BTreeNode *x, int i) {
    BTreeNode *y = x->child[i];
    BTreeNode *z = new_node(y->leaf);

    // z takes the upper half of y
    z->n = MIN_KEYS;
    for (int j = 0; j < MIN_KEYS; j++) copy_key(z, j, y, j + MIN_DEGREE);
    if (!y->leaf) {
        for (int j = 0; j < MIN_DEGREE; j++) z->child[j] = y->child[j + MIN_DEGREE];
    }
    y->n = MIN_KEYS;

    // make room in x for z and the middle key
    for (int j = x->n; j > i; j--) x->child[j + 1] = x->child[j];
    x->child[i + 1] = z;
    for (int j = x->n - 1; j >= i; j--) copy_key(x, j + 1, x, j);
    copy_key(x, i, y, MIN_DEGREE - 1);
    x->n++;
}

static void insert_non_full(BTreeNode *x, const char *key, int val) {
    int i = x->n - 1;

    if (x->leaf) {
        // shift bigger keys right, then put the key in its place
        while (i >= 0 && strcmp(key, x->keys[i]) < 0) {
            copy_key(x, i + 1, x, i);
            i--;
        }
        strcpy(x->keys[i + 1], key);
        x->vals[i + 1] = val;
        x->n++;
        return;
    }

    // find the child to go down into
    while (i >= 0 && strcmp(key, x->keys[i]) < 0) i--;
    i++;
    // split it first if it is full
    if (x->child[i]->n == MAX_KEYS) {
        split_child(x, i);
        if (strcmp(key, x->keys[i]) > 0) i++;
    }
    insert_non_full(x->child[i], key, val);
}

void btree_insert(BTree *t, const char *key, int val) {
    // full root: split it, the tree grows one level taller
    if (t->root->n == MAX_KEYS) {
        BTreeNode *s = new_node(false);
        s->child[0] = t->root;
        t->root = s;
        split_child(s, 0);
    }
    insert_non_full(t->root, key, val);
    t->count++;
}

// ---------------- check ----------------

static bool check_node(BTreeNode *x, bool is_root, int depth, int *leaf_depth) {
    if (x->n > MAX_KEYS) return false;
    if (!is_root && x->n < MIN_KEYS) return false;

    if (x->leaf) {
        if (*leaf_depth == -1) *leaf_depth = depth;
        return depth == *leaf_depth;   // all leaves at the same depth
    }
    for (int i = 0; i <= x->n; i++) {
        if (!check_node(x->child[i], false, depth + 1, leaf_depth)) return false;
    }
    return true;
}

// in-order walk: every key must be bigger than the one before it
static bool check_order(BTreeNode *x, const char **prev, int *seen) {
    for (int i = 0; i < x->n; i++) {
        if (!x->leaf && !check_order(x->child[i], prev, seen)) return false;
        if (*prev != NULL && strcmp(*prev, x->keys[i]) >= 0) return false;
        *prev = x->keys[i];
        (*seen)++;
    }
    if (!x->leaf) return check_order(x->child[x->n], prev, seen);
    return true;
}

bool btree_check(BTree *t) {
    if (!t->root->leaf && t->root->n == 0) return false;
    int leaf_depth = -1;
    if (!check_node(t->root, true, 0, &leaf_depth)) return false;

    const char *prev = NULL;
    int seen = 0;
    if (!check_order(t->root, &prev, &seen)) return false;
    return seen == t->count;
}
