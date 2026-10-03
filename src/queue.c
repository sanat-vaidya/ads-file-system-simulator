#include "queue.h"

void queue_init(Queue *q) {
    q->head = 0;
    q->tail = 0;
    q->count = 0;
}

bool queue_empty(Queue *q) {
    return q->count == 0;
}

bool queue_full(Queue *q) {
    return q->count == QUEUE_CAP;
}

void enqueue(Queue *q, int x) {
    q->items[q->tail] = x;
    q->tail = (q->tail + 1) % QUEUE_CAP;
    q->count++;
}

int dequeue(Queue *q) {
    int x = q->items[q->head];
    q->head = (q->head + 1) % QUEUE_CAP;
    q->count--;
    return x;
}
