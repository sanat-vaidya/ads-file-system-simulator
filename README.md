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

