#ifndef FAT16_SUPER_H
#define FAT16_SUPER_H

#include <linux/fs.h>

/**
 * fat16_superblock - in-memory superblock for fat16
 *
 * The fields are auto explicative.
 */
struct fat16_super {
	u32 bytes_per_sector;
	u32 sectors_per_cluster;
	u32 reserved_sectors;
	u32 number_of_fats;
	u32 root_entries;
	u32 total_sectors;
	u32 sectors_per_fat;
	u32 root_dir_sectors;
	u32 fat_offset;
	u32 root_dir_offset;
	u32 data_offset;
	u32 data_sectors;
	u32 total_clusters;
};

/**
* fat16_bpb - in-disk BIOS Parameter Block
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

/* superblock operations for fat16 */
extern const struct super_operations fat16_super_operations;

int fat16_superblock_fill(struct super_block *sb, void *data, int silent);

void fat16_superblock_kill(struct super_block *sb);

void fat16_superblock_put(struct super_block *sb);


#endif
