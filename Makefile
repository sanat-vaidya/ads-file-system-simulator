CC = gcc
CFLAGS = -std=c99 -Wall -Wextra -g

tests/btree_test: tests/btree_test.c src/btree.c src/btree.h
	$(CC) $(CFLAGS) -o tests/btree_test tests/btree_test.c src/btree.c

test: tests/btree_test
	./tests/btree_test

clean:
	rm -f tests/btree_test

.PHONY: test clean
