// btree_test.c - random stress test for the B-tree
//
// Does thousands of random inserts, and after every step checks:
//  - all B-tree rules hold (btree_check)
//  - the contents match a simple reference: an array saying which keys are in

#include "../src/btree.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define OPS 20000
#define POOL 300   // keys are picked from f0 .. f299

static char names[POOL][NAME_SIZE];
static int sorted_idx[POOL];   // key numbers in string order
static bool present[POOL];
static int value[POOL];
static int vals[POOL];

static int compare_names(const void *a, const void *b) {
    return strcmp(names[*(const int *)a], names[*(const int *)b]);
}

static int fail(int step, const char *why) {
    printf("FAIL step %d: %s\n", step, why);
    return 1;
}

int main(void) {
    srand(42);
    for (int i = 0; i < POOL; i++) {
        sprintf(names[i], "f%d", i);
        sorted_idx[i] = i;
    }
    qsort(sorted_idx, POOL, sizeof(int), compare_names);

    BTree *t = btree_new();

    for (int step = 0; step < OPS; step++) {
        int k = rand() % POOL;
        if (btree_find(t, names[k]) == -1) {
            int v = k * 1000 + rand() % 1000;   // value also tells us the key
            btree_insert(t, names[k], v);
            present[k] = true;
            value[k] = v;
        }

        if (!btree_check(t)) return fail(step, "B-tree rules broken");
        if (btree_find(t, names[k]) != value[k]) return fail(step, "find gave a wrong value");

        // in-order list must be exactly the present keys, in string order
        int n = btree_list(t, vals);
        int j = 0;
        for (int s = 0; s < POOL; s++) {
            int idx = sorted_idx[s];
            if (!present[idx]) continue;
            if (j >= n || vals[j] != value[idx]) return fail(step, "contents differ");
            j++;
        }
        if (j != n) return fail(step, "extra keys in the tree");
    }
    btree_free(t);

    printf("btree_test: %d random inserts OK\n", OPS);
    return 0;
}
