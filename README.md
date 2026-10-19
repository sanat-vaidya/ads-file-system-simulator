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

## Design

The system has two layers, like a real file system:

- **Namespace**: names, directories and paths.
- **Storage**: a simulated disk of 64 blocks x 32 bytes, plus the inodes.

An **inode** (`src/inode.h`) is where the two meet. It holds a name, a parent id,
and either a B-tree of entries (directory) or the first disk block (file).

There is one disk, one inode table and one trie, so each is kept as global data
inside its own `.c` file. There are many B-trees (one per directory), so B-tree,
stack and queue functions take a pointer to the structure they work on.
Strings and lists use fixed-size arrays; `malloc` is only used for nodes
(inodes, B-tree nodes, trie nodes, hash and id list entries).

### Data structures

| Structure | File | Job | Cost |
|---|---|---|---|
| Tree | `inode.h`, `fs.c` | directory hierarchy: B-trees point down, `parent_id` points up | |
| B-tree (max 3 keys/node) | `btree.c` | entries of one directory, name -> inode id | O(log n) find, insert, remove |
| Hash map (31 buckets, chaining) | `inode_table.c` | inode table, id -> inode | O(1) average |
| Trie | `trie.c` | index of every name, for `search` | O(prefix + nodes below it) |
| Bitmap | `disk.c` | which blocks are free, 1 bit per block | O(blocks) to scan |
| FAT (linked list in an array) | `disk.c` | each file's chain of blocks | O(chain length) to follow |
| Stack (array) | `stack.c` | cleaning up paths with `..` and `.`, building `pwd` | O(1) push/pop |
| Queue (circular array) | `queue.c` | breadth-first `find` | O(1) enqueue/dequeue |

Depth-first recursion is used by `tree`, `rm -r` and `diskmap`.

### What `write /docs/a.txt hello` does

1. **Stack**: the path becomes the clean list `[docs, a.txt]`.
2. **B-tree** of `/` gives the id of `docs`; the **hash map** gives its inode.
3. `docs`'s **B-tree** has no `a.txt`, so a new inode is made and added to the
   B-tree, the **hash map** and the **trie**.
4. 5 bytes need 1 block: the **bitmap** finds the lowest free block.
5. The **FAT** marks it as the end of the chain, and the bytes are copied in.

### Rules and limits

- 64 blocks of 32 bytes (2 KB), 128 inodes including `/`.
- Names: letters, digits, `.` `_` `-`, at most 32 characters, case-sensitive.
- Input lines: at most 4094 characters.
- `write` and `append` check for space first. If the disk is full nothing changes.
- New blocks are taken lowest number first, so deleting files leaves holes that
  later files fill. That is fragmentation, and the FAT handles it.
- Inode ids are never reused.
- Everything is in memory; nothing is saved when the program exits.

## Tests

- `tests/btree_test.c`: 20,000 random inserts and removes, checking all B-tree
  rules and comparing contents after every step.
- `tests/*.in` / `tests/*.out`: shell sessions and their expected output.
  `tests/run.sh` runs them all.
