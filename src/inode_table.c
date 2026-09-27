#include "inode_table.h"
#include "btree.h"
#include <stdio.h>
#include <stdlib.h>

static HashNode *buckets[NUM_BUCKETS];
static int count = 0;

int table_bucket_of(int id) {
    return id % NUM_BUCKETS;
}

int table_size(void) {
    return count;
}

void table_insert(Inode *inode) {
    int b = table_bucket_of(inode->id);
    HashNode *h = malloc(sizeof(HashNode));
    h->key = inode->id;
    h->inode = inode;
    h->next = buckets[b];
    buckets[b] = h;
    count++;
}

Inode *table_find(int id) {
    HashNode *h = buckets[table_bucket_of(id)];
    while (h != NULL) {
        if (h->key == id) return h->inode;
        h = h->next;
    }
    return NULL;
}

static void free_entry(HashNode *h) {
    btree_free(h->inode->children);   // NULL for files, that is fine
    free(h->inode);
    free(h);
}

void table_remove(int id) {
    int b = table_bucket_of(id);
    HashNode *h = buckets[b];
    HashNode *prev = NULL;
    while (h != NULL && h->key != id) {
        prev = h;
        h = h->next;
    }
    if (h == NULL) return;

    if (prev == NULL) buckets[b] = h->next;
    else prev->next = h->next;

    free_entry(h);
    count--;
}

void table_free_all(void) {
    for (int b = 0; b < NUM_BUCKETS; b++) {
        HashNode *h = buckets[b];
        while (h != NULL) {
            HashNode *next = h->next;
            free_entry(h);
            h = next;
        }
        buckets[b] = NULL;
    }
    count = 0;
}
