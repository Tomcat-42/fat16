#include <linux/init.h>
#include <linux/fs.h>
#include <linux/blkdev.h>
#include <linux/buffer_head.h>
#include <fat16/super.h>

#define FAT16_DISK_ENTRY_SIZE 32
#define FAT16_SUPER_MAGIC 0x41465331

/**
* fat16_bpb - BIOS Parameter Block
*
* This is the first sector of a FAT16 volume. It contains information
* about the volume, including the size of the FAT, the size of the
* root directory, and the size of the data area.
*/
struct fat16_bpb {
	u8 jmp[3];
	u8 oem[8];
	u16 bytes_per_sector;
	u8 sectors_per_cluster;
	u16 reserved_sectors;
	u8 number_of_fats;
	u16 root_entries;
	u16 total_sectors;
	u8 media_descriptor;
	u16 sectors_per_fat;
	u16 sectors_per_track;
	u16 number_of_heads;
	u32 hidden_sectors;
	u32 large_sectors;
	u8 drive_number;
	u8 reserved;
	u8 boot_signature;
	u32 volume_id;
	u8 volume_label[11];
	u8 system_id[8];
	u8 boot_code[448];
	u16 boot_sector_signature;
} __attribute__((packed));

struct inode *fat16_alloc_inode(struct super_block *sb)
{
	struct inode *inode = new_inode(sb);
	pr_info("fat16_alloc_inode called");
	if (!inode)
		return NULL;

	inode_init_owner(sb->s_user_ns, inode, NULL, S_IFREG);
	return inode;
}

void fat16_destroy_inode(struct inode *inode)
{
	pr_info("fat16_destroy_inode called");
	clear_inode(inode);
}

int fat16_write_inode(struct inode *inode, struct writeback_control *wbc)
{
	pr_info("fat16_write_inode called");
	return 0; // no writeback initially :(
}

void fat16_put_super(struct super_block *sb)
{
	kfree(sb->s_fs_info);
}

int fat16_statfs(struct dentry *dentry, struct kstatfs *buf)
{
	pr_info("fat16_statfs called");
	return 0; // TODO: do.
}

struct inode *fat16_iget(struct super_block *sb, unsigned long ino)
{
	struct inode *inode = NULL;

	if (!(inode = iget_locked(sb, ino))) {
		pr_err("fat16_iget: unable to get inode");
		return NULL;
	}

	// inode->i_ino = ino;
	// inode_init_owner(sb->s_user_ns, inode, NULL, S_IFDIR);
	// inode->i_op = &fat16_rootdir_inode_operations;
	// inode->i_fop = &fat16_rootdir_file_operations;
	// inode->i_blocks = inode->i_sb->s_blocksize;
	// inode->i_size = inode->i_sb->s_blocksize;

	return inode;
}

static const struct super_operations fat16_super_operations = {
	.alloc_inode = fat16_alloc_inode,
	.destroy_inode = fat16_destroy_inode,
	.write_inode = fat16_write_inode,
	.drop_inode = generic_delete_inode,
	.put_super = fat16_put_super,
	.statfs = fat16_statfs,
};

int fat16_fill_super(struct super_block *sb, void *data, int silent)
{
	struct buffer_head *bh;
	struct fat16_bpb *bpb;
	struct fat16_super_block_info *sbi;
	loff_t fat_offset, root_dir_offset, data_offset;
	void *fat = NULL;
	unsigned long fat_length = 0;
	unsigned long fat_sector = 0;
	unsigned long i = 0;
	// struct inode *root_inode = NULL, *fat_inode = NULL;

	/* Read the first block of the volume and cast it to a FAT BPB */
	sb_min_blocksize(sb, 512);
	if (!(bh = sb_bread(sb, 0))) {
		pr_err("fat16_fill_super: unable to read block 0");
		return -EINVAL;
	}
	bpb = (struct fat16_bpb *)bh->b_data;

	/* Check the boot_signature */
	if (bpb->boot_sector_signature != 0xAA55) {
		pr_err("fat16_fill_super: invalid boot sector signature");
		brelse(bh);
		return -EINVAL;
	}

	/* Initialization of the FAT16 superblock info */
	if (!(sbi = kzalloc(sizeof(struct fat16_super_block_info),
			    GFP_KERNEL))) {
		pr_err("fat16_fill_super: unable to allocate memory for sbi");
		brelse(bh);
		return -ENOMEM;
	}

	/* Calculate FAT16 specific values*/
	fat_offset = bpb->reserved_sectors * bpb->bytes_per_sector;
	root_dir_offset = fat_offset + bpb->number_of_fats *
					       bpb->sectors_per_fat *
					       bpb->bytes_per_sector;
	data_offset =
		root_dir_offset + bpb->root_entries * FAT16_DISK_ENTRY_SIZE;

	/* Fill the superblock info */
	sbi->fat_offset = fat_offset;
	sbi->root_dir_offset = root_dir_offset;
	sbi->data_offset = data_offset;
	sbi->sectors_per_cluster = bpb->sectors_per_cluster;
	sbi->fat_size_sectors = bpb->sectors_per_fat;
	sbi->cluster_size = bpb->sectors_per_cluster * bpb->bytes_per_sector;
	sbi->num_fats = bpb->number_of_fats;
	sbi->max_root_entries = bpb->root_entries;

	sb->s_magic = FAT16_SUPER_MAGIC;
	sb->s_op = &fat16_super_operations;

	sb->s_fs_info = sbi;

	pr_info("fat16_fill_super: sbi->fat_offset: %d", sbi->fat_offset);
	pr_info("fat16_fill_super: sbi->root_dir_offset: %d",
		sbi->root_dir_offset);
	/* Read FAT */
	fat_length = sbi->num_fats * sbi->fat_size_sectors;
	pr_info("fat16_fill_super: fat_length: %lu", fat_length);
	if (!(fat = kmalloc(fat_length * sb->s_blocksize, GFP_KERNEL))) {
		pr_err("fat16_fill_super: unable to allocate memory for fat");
		brelse(bh);
		return -ENOMEM;
	}

	/*
    * Read all FAT data
    * NOTE: Here I copy both FATs into the same buffer.
    */
	fat_sector = bpb->reserved_sectors;
	pr_info("fat16 reserved_sectors: %d", bpb->reserved_sectors);
	pr_info("fat16 fat_length: %lu", fat_length);
	for (i = 0; i < fat_length; i++) {
		if (!(bh = sb_bread(sb, fat_sector + i))) {
			pr_err("fat16_fill_super: unable to read block %lu",
			       sbi->fat_offset + i);
			kfree(fat);
			brelse(bh);
			return -EINVAL;
		}
		// pr_info("fat16 I am here");
		memcpy(fat + i * sb->s_blocksize, bh->b_data, sb->s_blocksize);
		brelse(bh);
	}

	/* Release the buffers */
	kfree(sbi);
	brelse(bh);
	return 0;
}

struct dentry *fat16_mount(struct file_system_type *fs_type, int flags,
			   const char *dev_name, void *data)
{
	pr_info("fat16_mount called");
	return mount_bdev(fs_type, flags, dev_name, data, fat16_fill_super);
}

void fat16_kill_sb(struct super_block *sb)
{
	pr_info("fat16_kill_sb called");
	kill_block_super(sb);
}
