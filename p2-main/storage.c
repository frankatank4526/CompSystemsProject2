/**
 * @file storage.c
 * Implementation of disk storage abstraction 
 */

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <errno.h>

#include "storage.h"

#include "blocks.h"
#include "inode.h"
#include "bitmap.h"

// retrieve inode index of given file 
int get_inode_index(const char *path) {
  slist_t * search_names = s_explode(path, '/');
  char* name_to_find = search_names->data;
  inode_t * inodes = get_inodes();

  // loops through the inode bitmap comparing the name of an inode and the given file
  // and returns the index if found
  for(int i = 0; i < 256; i++) {
    if(bitmap_get(get_inode_bitmap(), i)) {
      inode_t * inode_to_check = inodes + i * sizeof(inode_t);
        
        if(strcmp(inode_to_check->name, name_to_find)== 0){
          return i;
	}
	else{
	  continue;
	}
    } 
    else{
      continue;
    }		
  }
  return -ENOENT;
}

// set up the stat structure using inode metadata 
int storage_stat(const char *path, struct stat *st) {
  int inode_index = get_inode_index(path);
  if(inode_index < 0){
    st->st_nlink = 0;
    return -ENOENT;
  }
 
  inode_t * current_inode = get_inodes() + inode_index * sizeof(inode_t);
  st->st_mode = current_inode->mode;
  st->st_uid = current_inode->uid;
  st->st_size = current_inode->size;
  return 0;
}






