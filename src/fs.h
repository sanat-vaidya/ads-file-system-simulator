// fs.h - the file system: joins the namespace side (tree, B-tree, trie,
// stack, queue) with the storage side (disk, inode table).
// Every shell command is one function.

#ifndef FS_H
#define FS_H

#include <stdbool.h>
#include "inode.h"

#define LINE_SIZE 4096   // longest input line

// Longest path ever built: the current directory (at most MAX_INODES names
// of NAME_SIZE chars each) + "/" + one input line.
#define PATH_SIZE (MAX_INODES * NAME_SIZE + LINE_SIZE)

void fs_init(void);
void fs_shutdown(void);   // frees everything

// namespace
void fs_mkdir(const char *path);
void fs_touch(const char *path);
void fs_ls(const char *path);
void fs_cd(const char *path);
void fs_pwd(void);

void fs_cwd_path(char *out);   // out needs PATH_SIZE chars

#endif
