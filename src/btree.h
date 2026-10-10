// btree.h - B-tree from name to inode id, one per directory
//
// Each node holds 1 to 3 sorted keys (the root may hold 0). An internal
// node with k keys has k + 1 children. All leaves are at the same depth,
// so find, insert and remove are O(log n).
//
// Real file systems (ext4, NTFS, Btrfs) index large directories this way.
// We use a tiny node size (max 3 keys) so splits and merges show up after
// only a few files. Use the `btree` command to see one.
//
// Insert: split any full node on the way down, so there is always room.
// Remove: before going down into a child, make sure it has a spare key,
// by borrowing one from a sibling or merging with a sibling.
// (Cormen et al., Introduction to Algorithms, chapter 18.)
//
// There is one B-tree per directory, so every function takes a BTree pointer.

#ifndef BTREE_H
#define BTREE_H

#include <stdbool.h>
#include "inode.h"   // NAME_SIZE

#define MIN_DEGREE 2                    // t
#define MAX_KEYS (2 * MIN_DEGREE - 1)   // 3
#define MIN_KEYS (MIN_DEGREE - 1)       // 1 (except the root)

typedef struct BTreeNode {
    int n;                                // number of keys in use
    char keys[MAX_KEYS][NAME_SIZE];
    int vals[MAX_KEYS];
    struct BTreeNode *child[MAX_KEYS + 1];
    bool leaf;
} BTreeNode;

typedef struct BTree {
    BTreeNode *root;
    int count;
} BTree;

BTree *btree_new(void);
void btree_free(BTree *t);   // does nothing for NULL

int btree_find(BTree *t, const char *key);              // value, or -1 if missing
void btree_insert(BTree *t, const char *key, int val);  // key must be new, at most MAX_NAME chars
bool btree_remove(BTree *t, const char *key);           // false if missing

int btree_size(BTree *t);
int btree_height(BTree *t);
int btree_list(BTree *t, int vals[]);   // values in key order, returns how many
void btree_print(BTree *t);
bool btree_check(BTree *t);             // verifies all B-tree rules (used by tests)

#endif
