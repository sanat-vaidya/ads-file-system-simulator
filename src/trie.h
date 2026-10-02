// trie.h - global index of every name in the file system
//
// Each node has one child slot per allowed character. A name is a path
// from the root down; the node where it ends keeps a linked list of the
// ids of every inode with that name (the same name can exist in many
// directories).
//
// search(prefix) walks down the prefix, then collects everything below.
// Cost: O(length of prefix + nodes below it), no matter how many files
// exist. Compare with `find`, which has to visit every inode.
//
// Remove deletes nodes that end up with no ids and no children,
// so the trie never keeps dead branches.
//
// There is only one trie, so its root is a global inside trie.c.

#ifndef TRIE_H
#define TRIE_H

// allowed name characters, in ASCII order so results come out sorted
#define ALPHABET "-.0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ_abcdefghijklmnopqrstuvwxyz"
#define ALPHABET_SIZE 65

typedef struct IdNode {
    int id;
    struct IdNode *next;
} IdNode;

typedef struct TrieNode {
    struct TrieNode *child[ALPHABET_SIZE];
    IdNode *ids;   // inodes whose name ends here, oldest first
} TrieNode;

void trie_init(void);
void trie_insert(const char *name, int id);
void trie_remove(const char *name, int id);
int trie_search(const char *prefix, int out[]);   // fills out[] with ids, returns how many
void trie_free_all(void);

#endif
