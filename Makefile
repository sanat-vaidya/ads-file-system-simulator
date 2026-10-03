CC = gcc
CFLAGS = -std=c99 -Wall -Wextra -g

SRC = src/main.c src/fs.c src/disk.c src/inode_table.c src/btree.c src/trie.c src/stack.c
HEADERS = $(wildcard src/*.h)

fs: $(SRC) $(HEADERS)
	$(CC) $(CFLAGS) -o fs $(SRC)

tests/btree_test: tests/btree_test.c src/btree.c src/btree.h
	$(CC) $(CFLAGS) -o tests/btree_test tests/btree_test.c src/btree.c

test: fs tests/btree_test
	./tests/btree_test
	./tests/run.sh

clean:
	rm -f fs tests/btree_test

.PHONY: test clean
