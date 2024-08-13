// based on cs3650 starter code

#include <assert.h>
#include <bsd/string.h>
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define FUSE_USE_VERSION 26
#include <fuse.h>
#include "blocks.h"
#include "inode.h"
#include "storage.h"
#include "slist.h"
#include "bitmap.h"

// implementation for: man 2 access
// Checks if a file exists.
// TODO: add logic for if mask is 0 or 1 i.e. not just if inode exists,
// but also if it is the correct state (use bitmap) 
int nufs_access(const char *path, int mask) {
  int rv = 0;
  if(mask == 1){ //for read/write case -- file needs to already exist 
    if(bitmap_get(get_inode_bitmap(), get_inode_index(path)) != 1){
      return -ENOENT;
    }
    else{
      return 0;
    }
  }
  if(get_inode_index(path) >= 0){
    rv = 0;
  }
  else { // ...others do not exist
    rv = -ENOENT;
  }

  printf("access(%s, %04o) -> %d\n", path, mask, rv);
  return rv;
}

// Gets an object's attributes (type, permissions, size, etc).
// Implementation for: man 2 stat
// This is a crucial function.
int nufs_getattr(const char *path, struct stat *st) {
	int rv = 0;
	memset(st, 0, sizeof(struct stat));

	int inode_index = get_inode_index(path);
	if(inode_index < 0){
	  printf("file in given path not found \n ");
	  return -ENOENT;
	}

	inode_t * current_inode = (inode_t*) get_inodes() + inode_index * sizeof(inode_t);
	st->st_mode = current_inode->mode;
	st->st_uid = current_inode->uid;
	st->st_size = current_inode->size;


	printf("getattr(%s) -> (%d) {mode: %04o, size: %ld}\n", path, rv, st->st_mode, st->st_size);
	return rv;
}

// implementation for: man 2 readdir
// lists the contents of a directory
int nufs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                 off_t offset, struct fuse_file_info *fi) {
  struct stat st;
  int rv;

  rv = nufs_getattr("/", &st);
  assert(rv == 0);
  slist_t * path_strings = s_explode(path, '/');

  while(path_strings->data != NULL) {
    rv = nufs_getattr(path_strings->data, &st);

    if(rv != 0) {
      	    break;
    }
    filler(buf, path_strings->data, &st, 0);
    path_strings = path_strings->next;
  }
  printf("readdir(%s) -> %d\n", path, rv);
  return 0;
}

// mknod makes a filesystem object like a file or directory
// called for: man 2 open, man 2 link
// Note, for this assignment, you can alternatively implement the create
// function.
int nufs_mknod(const char *path, mode_t mode, dev_t rdev) {
  int rv = 0;
  slist_t * path_strings = s_explode(path, '/');
  char* file_name = path_strings->data; 
  alloc_inode(file_name);

  printf("mknod(%s, %04o) -> %d\n", path, mode, rv);
  return rv;
}

// most of the following callbacks implement
// another system call; see section 2 of the manual
int nufs_mkdir(const char *path, mode_t mode) {
  int rv = nufs_mknod(path, mode | 040000, 0);
  printf("mkdir(%s) -> %d\n", path, rv);
  return rv;
}

int nufs_unlink(const char *path) {
  int rv = -1;
  printf("unlink(%s) -> %d\n", path, rv);
  return rv;
}

int nufs_link(const char *from, const char *to) {
  int rv = -1;
  printf("link(%s => %s) -> %d\n", from, to, rv);
  return rv;
}

int nufs_rmdir(const char *path) {
  int rv = -1;
  printf("rmdir(%s) -> %d\n", path, rv);
  return rv;
}

// implements: man 2 rename
// called to move a file within the same filesystem
int nufs_rename(const char *from, const char *to) {
  int rv = -1;
  printf("rename(%s => %s) -> %d\n", from, to, rv);
  return rv;
}

int nufs_chmod(const char *path, mode_t mode) {
  int rv = -1;
  printf("chmod(%s, %04o) -> %d\n", path, mode, rv);
  return rv;
}

int nufs_truncate(const char *path, off_t size) {
  int rv = -1;
  printf("truncate(%s, %ld bytes) -> %d\n", path, size, rv);
  return rv;
}

// This is called on open, but doesn't need to do much
// since FUSE doesn't assume you maintain state for
// open files.
// You can just check whether the file is accessible.
int nufs_open(const char *path, struct fuse_file_info *fi) {
  int rv = 0;
  rv = nufs_access(path, 1);
  if (rv < 0){
    nufs_mknod(path, 0, 0); 		
  }
  printf("open(%s) -> %d\n", path, rv);
  return rv;
}

// Actually read data
int nufs_read(const char *path, char *buf, size_t size, off_t offset,
              struct fuse_file_info *fi) {
  int rv = 0;
  if(nufs_access(path, 1) < 0){
    return -ENOENT;
  }
  else {
    int inode_index = get_inode_index(path);
    inode_t* target_inode = get_inode(inode_index);
    char * block_data =(char *) target_inode->block;
    memcpy(buf, block_data + offset, size); //assuming size = how many characters to read, not bytes
    printf("read from file: %s \n", buf);
    printf("read(%s, %ld bytes, @+%ld) -> %d\n", path, size, offset, rv);
    return rv;
  }
}

// Actually write data
int nufs_write(const char *path, const char *buf, size_t size, off_t offset,
               struct fuse_file_info *fi) {
  int rv = 0;
  if(nufs_access(path, 1) < 0){
    return -ENOENT;
  }
  else {
    int inode_index = get_inode_index(path);
    inode_t* target_inode = get_inode(inode_index);
    char * block_data = (char *)target_inode->block;
    memcpy(block_data+offset, buf, size); 

    printf("Wrote %ld bytes \n", size);
    printf("write(%s, %ld bytes, @+%ld) -> %d\n", path, size, offset, rv);
    return rv;
  }
}

// Update the timestamps on a file or directory.
int nufs_utimens(const char *path, const struct timespec ts[2]) {
  int rv = -1;
  printf("utimens(%s, [%ld, %ld; %ld %ld]) -> %d\n", path, ts[0].tv_sec,
         ts[0].tv_nsec, ts[1].tv_sec, ts[1].tv_nsec, rv);
  return rv;
}

// Extended operations
int nufs_ioctl(const char *path, int cmd, void *arg, struct fuse_file_info *fi,
               unsigned int flags, void *data) {
  int rv = -1;
  printf("ioctl(%s, %d, ...) -> %d\n", path, cmd, rv);
  return rv;
}

void nufs_init_ops(struct fuse_operations *ops) {
  memset(ops, 0, sizeof(struct fuse_operations));
  ops->access = nufs_access;
  ops->getattr = nufs_getattr;
  ops->readdir = nufs_readdir;
  ops->mknod = nufs_mknod;
  ops->mkdir = nufs_mkdir;
  ops->link = nufs_link;
  ops->unlink = nufs_unlink;
  ops->rmdir = nufs_rmdir;
  ops->rename = nufs_rename;
  ops->chmod = nufs_chmod;
  ops->truncate = nufs_truncate;
  ops->open = nufs_open;
  ops->read = nufs_read;
  ops->write = nufs_write;
  ops->ioctl = nufs_ioctl;
};

struct fuse_operations nufs_ops;

int main(int argc, char *argv[]) {
  assert(argc > 2 && argc < 6);
  blocks_init(argv[--argc]);
  nufs_init_ops(&nufs_ops);
  return fuse_main(argc, argv, &nufs_ops, NULL);
}
