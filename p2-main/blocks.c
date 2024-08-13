/**
 * @file blocks.c
 * @author CS3650 staff
 *
 * Implementatino of a block-based abstraction over a disk image file.
 */

#define _GNU_SOURCE
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "inode.h"
#include "bitmap.h"
#include "blocks.h"

void *disk;
const int SIZE_OF_DISK = 1024 * 1024; // 1MB
const int BLOCK_COUNT = 256; // we split the "disk" into 256 blocks
const int BLOCK_SIZE = 4096; // = 4K
const int NUFS_SIZE = BLOCK_SIZE * BLOCK_COUNT; // = 1MB
const int BLOCK_BITMAP_SIZE = BLOCK_COUNT / 8; 
// Note: assumes block count is divisible by 8

static int blocks_fd = -1;
static void *blocks_base = 0;

// Get the number of blocks needed to store the given number of bytes.
int bytes_to_blocks(int bytes) {
  int quo = bytes / BLOCK_SIZE;
  int rem = bytes % BLOCK_SIZE;
  if (rem == 0) {
    return quo;
  } else {
    return quo + 1;
  }
}

// Load and initialize the given disk image.
void blocks_init(const char *image_path) {
  
  blocks_fd = open(image_path, O_CREAT | O_RDWR, 0644);
  assert(blocks_fd != -1);

  // make sure the disk image is exactly 1MB
  int rv = ftruncate(blocks_fd, NUFS_SIZE);
  assert(rv == 0);

  // map the image to memory
  blocks_base =
      mmap(0, NUFS_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, blocks_fd, 0);
  assert(blocks_base != MAP_FAILED);

  // block 0 stores the block bitmap and the inode bitmap
  void *bbm = get_blocks_bitmap();
  bitmap_put(bbm, 0, 1);
}

// Close the disk image.
void blocks_free() {
  int rv = munmap(blocks_base, NUFS_SIZE);
  assert(rv == 0);
}

// Get the given block, returning a pointer to its start.
void *blocks_get_block(int bnum) { return blocks_base + BLOCK_SIZE * bnum; }

// Return a pointer to the beginning of the block bitmap.
// The size is BLOCK_BITMAP_SIZE bytes.
void *get_blocks_bitmap() { return blocks_get_block(0); }

// Return a pointer to the beginning of the inode table bitmap.
void *get_inode_bitmap() {
  uint8_t *block = blocks_get_block(0);

  // The inode bitmap is stored immediately after the block bitmap
  return (void *) (block + BLOCK_BITMAP_SIZE);
}

// returns the start of the inode table
void * get_inodes() {
  uint8_t *block = blocks_get_block(0);

  return (void*) (block + BLOCK_BITMAP_SIZE * 2);
}

// returns the start of the block table
void * get_blocks() {
  uint8_t *block = blocks_get_block(0);
   
  return (void *) (block + BLOCK_BITMAP_SIZE * 2 + (BLOCK_COUNT * sizeof(inode_t)));
}



// finds and returns the next free inode in the inode table bitmap 
int free_inode_index() {
  for(int i = 0; i < BLOCK_COUNT; i++){
    if(bitmap_get(get_inode_bitmap(), i) == 0){
      return i;
    }
  }
  printf("no free inodes \n");
  return -ENOENT;
}

// finds and returns the next free block in the block table bitmap
int free_block_index() {
  for(int i = 0; i < BLOCK_COUNT; i++){
    if(bitmap_get(get_blocks_bitmap(), i) == 0){
      return i;
    }
  }
  printf("no free blocks \n");
  return -ENOENT;
}

// Allocate a new inode and return its index.
int alloc_inode_blocks() {
  void *ibm = get_inode_bitmap();

  for (int ii = 1; ii < BLOCK_COUNT; ++ii) {
    if (!bitmap_get(ibm, ii)) {
      bitmap_put(ibm, ii, 1);
      printf("+ alloc_inode() -> %d\n", ii);
      return ii;
    }
  }

  return -1;
}


// Allocate a new block and return its index.
int alloc_block() {
  void *bbm = get_blocks_bitmap();

  for (int ii = 1; ii < BLOCK_COUNT; ++ii) {
    if (!bitmap_get(bbm, ii)) {
      bitmap_put(bbm, ii, 1);
      printf("+ alloc_block() -> %d\n", ii);
      return ii;
    }
  }

  return -1;
}

// Deallocate the block with the given index.
void free_block(int bnum) {
  printf("+ free_block(%d)\n", bnum);
  void *bbm = get_blocks_bitmap();
  bitmap_put(bbm, bnum, 0);
}


// Deallocate the inode with the given index.
void free_inode_blocks(int inum) {
  printf("+ free_inode(%d)\n", inum);
  void *ibm = get_inode_bitmap();
  bitmap_put(ibm, inum, 0);
}

