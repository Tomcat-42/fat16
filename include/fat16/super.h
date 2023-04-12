#include <linux/fs.h>

// struct fat16_superblock {
// 	uint32_t fat_offset;
// 	uint16_t sectors_per_cluster;
// 	uint32_t first_data_sector;
// 	uint32_t data_sectors;
// 	uint32_t data_clusters;
// 	uint32_t root_dir_sectors;
// 	uint32_t root_dir_offset;
// };

struct fat16_super_block_info {
	u32 fat_offset; // Offset of the first FAT
	u32 root_dir_offset; // Offset of the root directory
	u32 data_offset; // Offset of the data area
	u32 sectors_per_cluster; // Number of sectors per cluster
	u32 fat_size_sectors; // Number of sectors per FAT
	u32 cluster_size; // Size of a cluster in bytes
	u32 num_fats; // Number of FATs
	u32 max_root_entries; // Maximum number of root directory entries
};

/**
 * fa16_fill_super - fill a superblock with a fat16 filesystem
 * @sb: superblock to fill
 * @data: data passed to mount
 * @silent: don't print errors
 */
int fat16_fill_super(struct super_block *sb, void *data, int silent);

/**
 * fat16_mount - mount a fat16 filesystem
 * @fs_type: the filesystem type
 * @flags: mount flags
 * @dev_name: device name
 * @data: data passed to mount
 */
struct dentry *fat16_mount(struct file_system_type *fs_type, int flags,
			   const char *dev_name, void *data);

/**
* fat16_kill_sb - kill a fat16 superblock
  * @sb: superblock to kill
  */
void fat16_kill_sb(struct super_block *sb);
