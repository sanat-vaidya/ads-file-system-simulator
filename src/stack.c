#include "stack.h"

void stack_init(Stack *s) {
    s->top = -1;
}

bool stack_empty(Stack *s) {
    return s->top == -1;
}

bool stack_full(Stack *s) {
    return s->top == STACK_CAP - 1;
}

int stack_size(Stack *s) {
    return s->top + 1;
}

void stack_push(Stack *s, const char *item) {
    s->top++;
    s->items[s->top] = item;
}

const char *stack_pop(Stack *s) {
    const char *item = s->items[s->top];
    s->top--;
    return item;
}
