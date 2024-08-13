/** 
 *  @file inode.h
 *
 *  Implementation of inode that holds file metadata
 *  based on cs3650 starter code
 *  */
#ifndef INODE_H
#define INODE_H

#include "blocks.h"

typedef struct inode {
  char* name; // file name
  int refs;  // reference count
  int mode;  // permission & type
  int size;  // bytes
   void * block; // single block pointer (if max file size <= 4K)
  int uid;

} inode_t;

// prints given inode data
void print_inode(inode_t *node);

// returns inode from inode table bitmap at given index
inode_t *get_inode(int inum);

// allocates a new inode
int alloc_inode(char * file_name);

// deallocates an inode at given index
void free_inode(int inum);

#endif
