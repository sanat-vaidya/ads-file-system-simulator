// disk.h - the simulated disk
//
// The disk is NUM_BLOCKS blocks of BLOCK_SIZE bytes each.
//
// Bitmap: one bit per block, 1 = used, 0 = free.
//   Block b is bit (b % 8) of byte (b / 8), so 64 blocks need only 8 bytes.
//
// FAT (File Allocation Table): fat[b] is the next block of the same file,
//   or FAT_END if b is the file's last block. So each file's blocks form a
//   linked list stored inside an array. An inode only remembers the first
//   block. The blocks do not need to be next to each other on the disk.
//
// There is only one disk, so its arrays are globals inside disk.c.

#ifndef DISK_H
#define DISK_H

#include <stdbool.h>

#define NUM_BLOCKS 64
#define BLOCK_SIZE 32
#define DISK_BYTES (NUM_BLOCKS * BLOCK_SIZE)   // the biggest a file can be
#define FAT_END (-1)    // end of a chain (also means "no blocks")
#define FAT_FREE (-2)   // block is not part of any chain

void disk_init(void);

// bitmap
bool disk_is_used(int block);
int disk_free_count(void);

// FAT chains
int disk_allocate(int count);
void disk_free_chain(int first);
void disk_link(int from, int to);
int disk_last_block(int first);
int disk_chain(int first, int out[]);   // fills out[] with the blocks, returns how many

// data
void disk_write_bytes(int first, int offset, const char *text, int len);
void disk_read_bytes(int first, int size, char *out);   // out gets size bytes + '\0'

// number of blocks needed to hold this many bytes
int blocks_for(int bytes);

#endif
