#ifndef FAT16_INODE_H
#define FAT16_INODE_H

#include <linux/fs.h>
#include <linux/init.h>

#include <fat16/dentry.h>

struct fat16_inode {
  struct inode vfs_inode;
  struct fat16_dentry *dentry;
};

#endif
