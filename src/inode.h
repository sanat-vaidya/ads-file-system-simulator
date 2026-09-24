// inode.h - shared by the namespace side and the storage side
//
// Every file and directory is one Inode.
//  - A directory keeps its entries in a B-tree (name -> inode id).
//  - A file keeps its bytes on the disk, in a chain starting at first_block.
// All inodes live in the inode table (a hash map from id to Inode).
// The directory tree is formed by the B-trees (parent -> children)
// and parent_id (child -> parent).

#ifndef INODE_H
#define INODE_H

#include <stdbool.h>

#define ROOT_ID 0
#define MAX_INODES 128            // including the root
#define MAX_NAME 32               // max characters in a name
#define NAME_SIZE (MAX_NAME + 1)  // room for the '\0'

struct BTree;

typedef struct Inode {
    int id;
    char name[NAME_SIZE];
    bool is_dir;
    int parent_id;            // the root is its own parent
    int size;                 // file size in bytes (files only)
    int first_block;          // first disk block of the file, -1 if empty
    struct BTree *children;   // directory entries (directories only, else NULL)
} Inode;

#endif
