// inode_table.h - hash map from inode id to Inode
//
// Separate chaining: NUM_BUCKETS buckets, each one a linked list.
// hash(id) = id % NUM_BUCKETS (a prime, so ids spread evenly).
// New entries go at the head of their bucket's list.
//
// Why not a plain array indexed by id? Ids are never reused, so they keep
// growing after deletes. An array would have to be as big as the largest
// id ever made. The hash map only holds live inodes and lookups stay
// O(1) on average.
//
// The table owns every inode: table_remove() frees it (and its B-tree).
// There is only one table, so it is a global inside inode_table.c.

#ifndef INODE_TABLE_H
#define INODE_TABLE_H

#include "inode.h"

#define NUM_BUCKETS 31

typedef struct HashNode {
    int key;   // inode id
    Inode *inode;
    struct HashNode *next;
} HashNode;

void table_insert(Inode *inode);
Inode *table_find(int id);
void table_remove(int id);

int table_size(void);
int table_bucket_of(int id);
int table_all_ids(int out[]);   // fills out[] with every id, returns how many
void table_free_all(void);

#endif
