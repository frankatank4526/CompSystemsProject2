/**
 * @file storage.h
 * Disk storage abstraction
 */

#ifndef NUFS_STORAGE_H
#define NUFS_STORAGE_H

#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include "slist.h"

// retrieve inode index of given file
int get_inode_index(const char *path);

// set up stat structure using inode data
int storage_stat(const char *path, struct stat *st);

#endif
