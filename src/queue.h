// queue.h - circular array queue of ints
//
// Used by `find` for breadth-first search: directories are visited level
// by level, so the closest matches come out first.
//
// head is the next item to remove, tail is the next free slot. Both wrap
// back to 0 at the end of the array, so freed slots get reused.
// enqueue and dequeue are O(1).

#ifndef QUEUE_H
#define QUEUE_H

#include <stdbool.h>

#define QUEUE_CAP 128   // >= MAX_INODES, each inode is queued at most once

typedef struct {
    int items[QUEUE_CAP];
    int head;
    int tail;
    int count;
} Queue;

void queue_init(Queue *q);
bool queue_empty(Queue *q);
bool queue_full(Queue *q);
void enqueue(Queue *q, int x);
int dequeue(Queue *q);

#endif
