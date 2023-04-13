#ifndef FAT16_FILE_H
#define FAT16_FILE_H

#include <linux/fs.h>
#include <linux/init.h>

#include <fat16/dentry.h>

enum fat16_file_type {
  FAT16_FILE_ATTR_FILE = 0x20,
  FAT16_FILE_ATTR_DIR = 0x10
};

struct fat16_file {
	struct file vfs_file;
  struct fat16_dentry *dentry;
};

struct fat16_dentry {
	u8 name[8];
	u8 ext[3];
	u8 attributes;
	u8 reserved;
	u8 creation_time_tenth;
	u16 creation_time;
	u16 creation_date;
	u16 last_access_date;
	u16 first_cluster_high;
	u16 last_modification_time;
	u16 last_modification_date;
	u16 first_cluster_low;
	u32 size;
} __attribute__((packed));

#endif
