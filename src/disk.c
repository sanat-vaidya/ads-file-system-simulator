#include "disk.h"

static char data[NUM_BLOCKS][BLOCK_SIZE];
static int fat[NUM_BLOCKS];
static unsigned char bitmap[NUM_BLOCKS / 8];

int blocks_for(int bytes) {
    return (bytes + BLOCK_SIZE - 1) / BLOCK_SIZE;
}

void disk_init(void) {
    for (int b = 0; b < NUM_BLOCKS; b++) {
        fat[b] = FAT_FREE;
        for (int i = 0; i < BLOCK_SIZE; i++) data[b][i] = 0;
    }
    for (int i = 0; i < NUM_BLOCKS / 8; i++) bitmap[i] = 0;
}

// ---------------- bitmap ----------------

bool disk_is_used(int block) {
    return (bitmap[block / 8] >> (block % 8)) & 1;
}

static void set_used(int block, bool used) {
    if (used) bitmap[block / 8] |= (1 << (block % 8));
    else bitmap[block / 8] &= ~(1 << (block % 8));
}

int disk_free_count(void) {
    int count = 0;
    for (int b = 0; b < NUM_BLOCKS; b++) {
        if (!disk_is_used(b)) count++;
    }
    return count;
}

// ---------------- FAT chains ----------------

// Take `count` free blocks (lowest numbers first) and link them into one chain.
// Returns the first block, or FAT_END if count is 0.
// The caller must check disk_free_count() first.
int disk_allocate(int count) {
    int first = FAT_END;
    int prev = FAT_END;
    for (int b = 0; b < NUM_BLOCKS && count > 0; b++) {
        if (disk_is_used(b)) continue;
        set_used(b, true);
        fat[b] = FAT_END;
        if (prev == FAT_END) first = b;
        else fat[prev] = b;
        prev = b;
        count--;
    }
    return first;
}

// Give every block of the chain back to the bitmap.
// The bytes stay on the disk until something overwrites them.
void disk_free_chain(int first) {
    int b = first;
    while (b != FAT_END) {
        int next = fat[b];
        set_used(b, false);
        fat[b] = FAT_FREE;
        b = next;
    }
}

void disk_link(int from, int to) {
    fat[from] = to;
}

int disk_last_block(int first) {
    int b = first;
    while (b != FAT_END && fat[b] != FAT_END) b = fat[b];
    return b;
}

int disk_chain(int first, int out[]) {
    int n = 0;
    for (int b = first; b != FAT_END; b = fat[b]) {
        out[n] = b;
        n++;
    }
    return n;
}
