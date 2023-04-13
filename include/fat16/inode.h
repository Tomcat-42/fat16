#ifndef FAT16_INODE_H
#define FAT16_INODE_H

#include <linux/fs.h>

struct fat16_inode {
  u32 i_start;
  struct inode vfs_inode;
};

#endif
