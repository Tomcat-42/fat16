#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/errno.h>

#include <fat16/super.h>
#include <fat16/dentry.h>

static struct file_system_type fat16_fs_type = { .owner = THIS_MODULE,
						 .name = "fat16",
						 .mount = fat16_dentry_mount,
						 .kill_sb = fat16_superblock_kill,
						 .fs_flags = FS_REQUIRES_DEV };

static int __init fat16_init(void)
{
	int ret;

	pr_info("fat16: registering filesystem");

	if ((ret = register_filesystem(&fat16_fs_type)) < 0) {
		pr_err("fat16: failed to register filesystem");
	}

	return ret;
}

static void __exit fat16_exit(void)
{
	int ret;

	pr_info("fat16: unregistering filesystem");

	if ((ret = unregister_filesystem(&fat16_fs_type)) < 0) {
		pr_err("fat16: failed to unregister filesystem");
	}
}

module_init(fat16_init);
module_exit(fat16_exit);

MODULE_ALIAS_FS("fat16");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Pablo Alessandro Santos Hugen <pablohuggem@gmail.com>");
MODULE_DESCRIPTION("FAT16 Filesystem Driver");
