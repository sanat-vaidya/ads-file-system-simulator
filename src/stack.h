// stack.h - array-based stack of names
//
// Used for paths:
//  1. Resolving "/a/b/../c": push each name, pop on "..", skip ".".
//     What is left is the clean path, with the last name on top.
//  2. pwd: walk parent links up from the current directory pushing
//     names, then pop them to build the path from the root down.
//
// The stack holds pointers to names that already exist somewhere else
// (in a path buffer or in an inode), so nothing is copied.
// push and pop are O(1).

#ifndef STACK_H
#define STACK_H

#include <stdbool.h>

#define STACK_CAP 256

typedef struct {
    const char *items[STACK_CAP];
    int top;   // index of the top item, -1 when empty
} Stack;

void stack_init(Stack *s);
bool stack_empty(Stack *s);
bool stack_full(Stack *s);
int stack_size(Stack *s);
void stack_push(Stack *s, const char *item);
const char *stack_pop(Stack *s);

#endif
