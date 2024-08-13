/**
 * @file inode.c
 *
 * Implementation of inode that holds file metadata
 */

#include <sys/stat.h>
#include <sys/mman.h>
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include "blocks.h"
#include "inode.h"

// prints given inode data 
void print_inode(inode_t *node) {
  printf("inode has name %s, with size %d", node->name, node->size);
}

// returns inode from inode table bitmap at given index
inode_t *get_inode(int inum) {
  return (inode_t*) get_inodes() + inum * sizeof(inode_t);
}

// allocate a new inode 
int alloc_inode(char* file_name){
  int free_inode = alloc_inode_blocks();
  int free_block = alloc_block();
  if(free_inode < 0 || free_block < 0){
    return -ENOENT;
  }	
		
  inode_t* new_inode = get_inode(free_inode);
  new_inode->mode = 0100644; //0 for file, 1 for directory
  new_inode->name = file_name;
  new_inode->size = BLOCK_SIZE; //currently only small sizes
  new_inode->block = get_blocks() + BLOCK_SIZE *free_block; //pointer to block
  new_inode->uid = getuid();
  return 0;
}

// deallocate inode at given index
void free_inode(int inum){
  inode_t* inode_to_free = get_inode(inum);
  free_block(inum);
  free_inode_blocks(inum);
  memset(inode_to_free->block, 0, BLOCK_SIZE);	
}




