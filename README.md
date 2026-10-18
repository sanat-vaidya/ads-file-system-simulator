# File System Simulator

A small in-memory file system with a Unix-like shell, written in C to show how
real file systems use data structures. Every data structure is hand-written;
only the C standard library is used.

## Build and run

```
make            # builds ./fs
./fs            # interactive shell
./fs < demo.txt # guided demo of every data structure
make test       # B-tree stress test + all shell tests
```

Needs `gcc` (C99) and `make` on Linux.

## Commands

| Command | What it does |
|---|---|
| `mkdir <path>...`, `touch <path>...` | make directories / empty files |
| `ls [path]`, `cd [path]`, `pwd` | list, change and print directory |
| `write <file> <text>` | replace a file's contents (creates it if needed) |
| `append <file> <text>` | add a line to the end of a file |
| `cat <file>` | print a file |
| `rm [-r] <path>...` | remove (`-r` for a directory and everything in it) |
| `mv <src> <dst>` | move or rename |
| `stat <path>` | show an inode |
| `search <prefix>` | every name starting with prefix, anywhere |
| `find <dir> <name>` | exact name under a directory, nearest first |
| `tree [path]` | print the directory tree |
| `df` | disk and inode usage |
| `diskmap` | which file owns each block, and the bitmap |
| `blocks <file>` | a file's block chain and what each block holds |
| `btree [dir]` | a directory's B-tree, level by level |
| `hashmap` | the inode table's buckets |

## Tests

- `tests/btree_test.c`: 20,000 random inserts and removes, checking all B-tree
  rules and comparing contents after every step.
- `tests/*.in` / `tests/*.out`: shell sessions and their expected output.
  `tests/run.sh` runs them all.
