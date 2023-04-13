#ifndef FAT16_DENTRY_H
#define FAT16_DENTRY_H

#include <linux/fs.h>

struct dentry *fat16_dentry_mount(struct file_system_type *fs_type, int flags,
				  const char *dev_name, void *data);

int fat16_statfs(struct dentry *dentry, struct kstatfs *buf);

int fat16_show_options(struct seq_file *m, struct dentry *root);

#endif
