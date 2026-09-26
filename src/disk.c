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

int disk_free_count(void) {
    int count = 0;
    for (int b = 0; b < NUM_BLOCKS; b++) {
        if (!disk_is_used(b)) count++;
    }
    return count;
}
