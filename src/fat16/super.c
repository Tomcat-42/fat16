#include <asm/unaligned.h>
#include <linux/buffer_head.h>
#include <linux/cred.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/pagemap.h>
#include <linux/sched.h>
#include <linux/slab.h>

#include <fat16/super.h>
#include <fat16/inode.h>
#include <fat16/dentry.h>
#include <fat16/file.h>

#define FAT16_DISK_ENTRY_SIZE 32
#define FAT16_SUPER_MAGIC 0x41465331
#define FAT16_SECTOR_SIZE 512
#define FAT16_ROOT_INODE 0
#define FAT16_TIME_GRANNULARITY 1

static struct file_operations fat16_file_operations = {
	// You can leave this empty if you don't need any specific file operations for files.
};

static struct inode_operations fat16_file_inode_operations = {
	// You can leave this empty if you don't need any specific inode operations for files.
};

static struct file_operations fat16_dir_operations = {
	// You can leave this empty if you don't need any specific file operations for directories.
};

static struct inode_operations fat16_dir_inode_operations = {
	// You can leave this empty if you don't need any specific inode operations for directories.
};

struct inode *fat16_inode_create(struct super_block *sb,
				 struct fat16_dentry *dentry_buf)
{
	struct inode *inode = NULL;
	unsigned int attributes;

	pr_info("fat16_inode_create: %s", __func__);
	if (!(inode = new_inode(sb))) {
		pr_err("fat16_inode_create: failed to create inode");
		return NULL;
	}

	attributes = dentry_buf->attributes;

	// Set the inode's attributes based on the dentry_buffer
	inode->i_ino = get_next_ino();
	inode->i_sb = sb;
	inode->i_atime = inode->i_mtime = inode->i_ctime = current_time(inode);
	inode->i_size = le32_to_cpu(dentry_buf->size);

	// Set the appropriate inode and file operations for files and directories
	if (attributes & FAT16_FILE_ATTR_DIR) {
		inode->i_mode = S_IFDIR | S_IRWXU | S_IRGRP | S_IXGRP |
				S_IROTH | S_IXOTH;
		inode->i_op = &fat16_dir_inode_operations;
		inode->i_fop = &fat16_dir_operations;
	} else if (attributes & FAT16_FILE_ATTR_FILE) {
		inode->i_mode = S_IFREG | S_IRWXU | S_IRGRP | S_IROTH;
		inode->i_op = &fat16_file_inode_operations;
		inode->i_fop = &fat16_file_operations;
	}

	return inode;
}

static int read_dentry_by_index(struct inode *dir,
				struct fat16_dentry *dentry_buffer, int index)
{
	struct super_block *sb = dir->i_sb;
	struct fat16_super *fs_info = sb->s_fs_info;
	loff_t entry_offset;
	struct buffer_head *bh;

	entry_offset = fs_info->root_dir_offset * fs_info->bytes_per_sector +
		       index * sizeof(struct fat16_dentry);
	bh = sb_bread(sb, entry_offset >> sb->s_blocksize_bits);
	if (!bh) {
		pr_err("failed to read rootdir block");
		return -EIO;
	}

	memcpy(dentry_buffer,
	       bh->b_data + (entry_offset & (sb->s_blocksize - 1)),
	       sizeof(struct fat16_dentry));
	brelse(bh);

	return 0;
}

static int fat16_iterate(struct file *filp, struct dir_context *ctx)
{
	struct inode *dir = file_inode(filp);
	struct super_block *sb = dir->i_sb;
	struct fat16_super *fs_info = sb->s_fs_info;
	int i;

	for (i = ctx->pos; i < fs_info->root_entries; i++) {
		struct fat16_dentry dentry_buffer;
		char name[9], ext[4], full_name[13];
		int name_len, ext_len;
		unsigned int type;
		struct inode *entry_inode;

		if (read_dentry_by_index(dir, &dentry_buffer, i) < 0) {
			return -EIO;
		}

		if (dentry_buffer.name[0] == 0x00 ||
		    dentry_buffer.name[0] == 0xE5 ||
		    !(dentry_buffer.attributes & FAT16_FILE_ATTR_DIR ||
		      dentry_buffer.attributes & FAT16_FILE_ATTR_FILE)) {
			ctx->pos++;
			continue;
		}

		type = dentry_buffer.attributes & FAT16_FILE_ATTR_DIR ? DT_DIR :
									DT_REG;

		strncpy(name, dentry_buffer.name, 8);
		strncpy(ext, dentry_buffer.name + 8, 3);
		name[8] = '\0';
		ext[3] = '\0';

		// Remove trailing spaces
		name_len = strnlen(name, sizeof(name)) - 1;
		ext_len = strnlen(ext, sizeof(ext)) - 1;
		while (name_len >= 0 && name[name_len] == ' ') {
			name[name_len--] = '\0';
		}
		while (ext_len >= 0 && ext[ext_len] == ' ') {
			ext[ext_len--] = '\0';
		}

		if (ext_len >= 0) {
			snprintf(full_name, sizeof(full_name), "%s.%s", name,
				 ext);
		} else {
			strncpy(full_name, name, sizeof(full_name));
		}

		entry_inode = fat16_inode_create(sb, &dentry_buffer);

		if (entry_inode) {
			if (!dir_emit(ctx, full_name, strlen(full_name),
				      entry_inode->i_ino, type)) {
				iput(entry_inode);
				break;
			}
			iput(entry_inode);
		}
		ctx->pos++;
	}

	return 0;
}

struct dentry *fat16_lookup(struct inode *dir, struct dentry *dentry,
			    unsigned int flags)
{
	struct fat16_super *sbi;
	struct fat16_dentry dentry_buf;
	struct inode *target_inode = NULL;
	int i;

	pr_info("fat16_lookup for file %s", dentry->d_name.name);

	sbi = (struct fat16_super *)dir->i_sb->s_fs_info;

	// Iterate through the directory entries to find the matching entry
	for (i = 0; i < sbi->root_entries; i++) {
		char name[9], ext[4], full_name[13];
		int name_len, ext_len;

		if (read_dentry_by_index(dir, &dentry_buf, i) < 0) {
			return ERR_PTR(-EIO);
		}

		strncpy(name, dentry_buf.name, 8);
		strncpy(ext, dentry_buf.name + 8, 3);
		name[8] = '\0';
		ext[3] = '\0';

		// Remove trailing spaces
		name_len = strnlen(name, sizeof(name)) - 1;
		ext_len = strnlen(ext, sizeof(ext)) - 1;
		while (name_len >= 0 && name[name_len] == ' ') {
			name[name_len--] = '\0';
		}
		while (ext_len >= 0 && ext[ext_len] == ' ') {
			ext[ext_len--] = '\0';
		}

		if (ext_len >= 0) {
			snprintf(full_name, sizeof(full_name), "%s.%s", name,
				 ext);
		} else {
			strncpy(full_name, name, sizeof(full_name));
		}

		// Compare the dentry name with the current entry's name
		if (strcmp(full_name, dentry->d_name.name) == 0) {
			target_inode =
				fat16_inode_create(dir->i_sb, &dentry_buf);
			break;
		}
	}

	if (target_inode) {
		d_add(dentry, target_inode);
		return NULL;
	}

	return ERR_PTR(-ENOENT);
}

const struct super_operations fat16_super_operations = {
	.statfs = simple_statfs,
	.drop_inode = generic_delete_inode,
	.show_options = fat16_show_options,
};

const struct inode_operations fat16_rootdir_inode_operations = {
	.lookup = fat16_lookup,
};

const struct file_operations fat16_rootdir_file_operations = {
	.llseek = generic_file_llseek,
	.read = generic_read_dir,
	.iterate_shared = fat16_iterate,
	.iterate = fat16_iterate,
	.fsync = generic_file_fsync,
};

int fat16_superblock_fill(struct super_block *sb, void *data, int silent)
{
	struct buffer_head *bh = NULL; // block buffer
	struct fat16_bpb *bpb = NULL;
	struct fat16_super *sbi = NULL;

	struct inode *root_inode = NULL;
	struct dentry *root_dentry = NULL;
	// int i;

	/* Initialization of the FAT16 superblock info */
	if (!(sbi = kzalloc(sizeof(struct fat16_super), GFP_KERNEL))) {
		pr_err("fat16_fill_super: unable to allocate memory for sbi");
		brelse(bh);
		return -ENOMEM;
	}

	/* Read the block #0 (BPB) */
	sb_min_blocksize(sb, 512);
	if (!(bh = sb_bread(sb, 0))) {
		pr_err("fat16_fill_super: unable to read block 0");
		return -EINVAL;
	}
	/* in the image I'm using for testing, the following values are:
    * bytes_per_sector: 512
    * sectors_per_cluster: 1
    * reserved_sectors: 1
    * number_of_fats: 2
    * root_entries: 512
    * total_sectors: 40000
    * sectors_per_fat: 155
    */
	bpb = (struct fat16_bpb *)bh->b_data;

	/* Check the boot_signature */
	if (bpb->boot_sector_signature != 0xAA55) {
		pr_err("fat16_fill_super: invalid boot sector signature");
		brelse(bh);
		return -EINVAL;
	}

	/* Store relevant info in the superblock */
	sbi->bytes_per_sector =
		le16_to_cpu(get_unaligned_le16(&bpb->bytes_per_sector));
	sbi->sectors_per_cluster = bpb->sectors_per_cluster;
	sbi->reserved_sectors =
		le16_to_cpu(get_unaligned_le16(&bpb->reserved_sectors));
	sbi->number_of_fats = bpb->number_of_fats;
	sbi->root_entries = le16_to_cpu(get_unaligned_le16(&bpb->root_entries));
	sbi->total_sectors =
		le16_to_cpu(get_unaligned_le16(&bpb->total_sectors));
	sbi->sectors_per_fat =
		le16_to_cpu(get_unaligned_le16(&bpb->sectors_per_fat));
	sbi->root_dir_sectors = ((bpb->root_entries * FAT16_DISK_ENTRY_SIZE) +
				 (bpb->bytes_per_sector - 1)) /
				bpb->bytes_per_sector;
	sbi->fat_offset = sbi->reserved_sectors;
	sbi->root_dir_offset =
		sbi->fat_offset + sbi->number_of_fats * sbi->sectors_per_fat;
	sbi->data_offset = sbi->reserved_sectors +
			   sbi->number_of_fats * sbi->sectors_per_fat +
			   sbi->root_dir_sectors;
	sbi->data_sectors = sbi->total_sectors - sbi->data_offset;
	sbi->total_clusters = sbi->data_sectors / sbi->sectors_per_cluster;

	/* Print the info */
	pr_info("fat16_fill_super: root_dir_offset: %d", sbi->root_dir_offset);

	/* Relevant info for the superblock */
	sb->s_magic = FAT16_SUPER_MAGIC;
	sb->s_time_gran = FAT16_TIME_GRANNULARITY;
	sb->s_op = &fat16_super_operations;
	sb->s_fs_info = sbi;

	/* Creation of the root inode */
	if (!(root_inode = new_inode(sb))) {
		pr_err("fat16_fill_super: unable to create root inode");
		brelse(bh);
		return -EINVAL;
	}

	root_inode->i_ino = FAT16_ROOT_INODE;
	root_inode->i_sb = sb;
	root_inode->i_atime = root_inode->i_mtime = root_inode->i_ctime =
		current_time(root_inode);
	root_inode->i_op = &fat16_rootdir_inode_operations;
	root_inode->i_fop = &fat16_rootdir_file_operations;
	inode_init_owner(sb->s_user_ns, root_inode, NULL,
			 S_IFDIR | S_IRWXU | S_IRUGO | S_IXUGO);

	if (!(root_dentry = d_make_root(root_inode))) {
		pr_err("fat16_fill_super: unable to create root dentry");
		iput(root_inode);
		brelse(bh);
		return -EINVAL;
	}

	sb->s_root = root_dentry;
	// pr_info("fat16_fill_super: reading the first block of the root directory");
	// if (!(bh = sb_bread(sb, sbi->root_dir_offset))) {
	// 	pr_err("fat16_fill_super: unable to read the first block of the root directory");
	// 	brelse(bh);
	// 	return -EINVAL;
	// }

	//
	// root_inode->i_op = &fat16_rootdir_inode_operations;
	// root_inode->i_fop = &fat16_rootdir_file_operations;
	// root_inode->i_mode |= S_IFDIR;
	//
	// /* Creation of the root dentry */
	// if (!(root_dentry = d_make_root(root_inode))) {
	// 	pr_err("fat16_fill_super: unable to create root dentry");
	// 	iput(root_inode);
	// 	kfree(root_inode);
	// 	brelse(bh);
	// 	return -EINVAL;
	// }
	//
	// sb->s_root = root_dentry;

	// /* Read FAT */
	// fat_length = sbi->num_fats * sbi->fat_size_sectors;
	// pr_info("fat16_fill_super: fat_length: %lu", fat_length);
	// if (!(fat = kmalloc(fat_length * sb->s_blocksize, GFP_KERNEL))) {
	// 	pr_err("fat16_fill_super: unable to allocate memory for fat");
	// 	brelse(bh);
	// 	return -ENOMEM;
	// }
	//
	// /*
	//    * Read all FAT data
	//    * NOTE: Here I copy both FATs into the same buffer.
	//    */
	// fat_sector = bpb->reserved_sectors;
	// pr_info("fat16 reserved_sectors: %d", bpb->reserved_sectors);
	// pr_info("fat16 fat_length: %lu", fat_length);
	// for (i = 0; i < fat_length; i++) {
	// 	if (!(bh = sb_bread(sb, fat_sector + i))) {
	// 		pr_err("fat16_fill_super: unable to read block %lu",
	// 		       sbi->fat_offset + i);
	// 		kfree(fat);
	// 		brelse(bh);
	// 		return -EINVAL;
	// 	}
	// 	// pr_info("fat16 I am here");
	// 	memcpy(fat + i * sb->s_blocksize, bh->b_data, sb->s_blocksize);
	// 	brelse(bh);
	// }

	pr_info("fat16_fill_super: I am here %s:%d", __func__, __LINE__);

	/* Release the buffers */
	// kfree(sbi);
	brelse(bh);
	return 0;
}

void fat16_superblock_kill(struct super_block *sb)
{
	pr_info("fat16_kill_sb called");
	kill_block_super(sb);
}

void fat16_superblock_put(struct super_block *sb)
{
	kfree(sb->s_fs_info);
}
