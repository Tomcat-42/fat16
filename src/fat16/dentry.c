#include <linux/fs.h>
#include <linux/init.h>
#include <fat16/dentry.h>
#include <fat16/super.h>

struct dentry *fat16_dentry_mount(struct file_system_type *fs_type, int flags,
			   const char *dev_name, void *data)
{
	struct dentry *dentry = NULL;
	// NOTE: later I need proper error handling
	if (!(dentry = mount_bdev(fs_type, flags, dev_name, data,
				  fat16_superblock_fill))) {
		pr_err("fat16_mount: mount_bdev failed");
	}
	return dentry;
}

int fat16_statfs(struct dentry *dentry, struct kstatfs *buf)
{
	pr_info("fat16_statfs called");
	return 0; // TODO: do.
}

int fat16_show_options(struct seq_file *m, struct dentry *root)
{
	pr_info("fat16_show_options called");
	return 0; // TODO: do.
}
