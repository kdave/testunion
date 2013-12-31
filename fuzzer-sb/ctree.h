/*
 * Copyright (C) 2007 Oracle.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License v2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 021110-1307, USA.
 */

#ifndef __BTRFS__
#define __BTRFS__

#include "kerncompat.h"

#define BTRFS_FSID_SIZE 16
#define BTRFS_UUID_SIZE 16
#define BTRFS_CSUM_SIZE 32

#define BTRFS_SUPER_INFO_OFFSET (64 * 1024)
#define BTRFS_SUPER_INFO_SIZE 4096

#define BTRFS_SUPER_MIRROR_MAX	 3
#define BTRFS_SUPER_MIRROR_SHIFT 12

#define BTRFS_CSUM_TYPE_CRC32	0
static int btrfs_csum_sizes[] = { 4, 0 };

struct btrfs_header {
	u8 csum[BTRFS_CSUM_SIZE];
	u8 fsid[BTRFS_FSID_SIZE];
	__le64 bytenr;
	__le64 flags;

	u8 chunk_tree_uuid[BTRFS_UUID_SIZE];
	__le64 generation;
	__le64 owner;
	__le32 nritems;
	u8 level;
} __attribute__ ((__packed__));

struct btrfs_dev_item {
	__le64 devid;
	__le64 total_bytes;
	__le64 bytes_used;
	__le32 io_align;
	__le32 io_width;
	__le32 sector_size;
	__le64 type;
	__le64 generation;
	__le64 start_offset;
	__le32 dev_group;
	u8 seek_speed;
	u8 bandwidth;
	u8 uuid[BTRFS_UUID_SIZE];
	u8 fsid[BTRFS_UUID_SIZE];
} __attribute__ ((__packed__));

#define BTRFS_NUM_BACKUP_ROOTS 4
struct btrfs_root_backup {
	__le64 tree_root;
	__le64 tree_root_gen;
	__le64 chunk_root;
	__le64 chunk_root_gen;
	__le64 extent_root;
	__le64 extent_root_gen;
	__le64 fs_root;
	__le64 fs_root_gen;
	__le64 dev_root;
	__le64 dev_root_gen;
	__le64 csum_root;
	__le64 csum_root_gen;
	__le64 total_bytes;
	__le64 bytes_used;
	__le64 num_devices;
	__le64 unsed_64[4];
	u8 tree_root_level;
	u8 chunk_root_level;
	u8 extent_root_level;
	u8 fs_root_level;
	u8 dev_root_level;
	u8 csum_root_level;
	u8 unused_8[10];
} __attribute__ ((__packed__));

#define BTRFS_SYSTEM_CHUNK_ARRAY_SIZE 2048
#define BTRFS_LABEL_SIZE 256

struct btrfs_super_block {
	u8 csum[BTRFS_CSUM_SIZE];
	u8 fsid[BTRFS_FSID_SIZE];
	__le64 bytenr;
	__le64 flags;
	__le64 magic;
	__le64 generation;
	__le64 root;
	__le64 chunk_root;
	__le64 log_root;
	__le64 log_root_transid;
	__le64 total_bytes;
	__le64 bytes_used;
	__le64 root_dir_objectid;
	__le64 num_devices;
	__le32 sectorsize;
	__le32 nodesize;
	__le32 leafsize;
	__le32 stripesize;
	__le32 sys_chunk_array_size;
	__le64 chunk_root_generation;
	__le64 compat_flags;
	__le64 compat_ro_flags;
	__le64 incompat_flags;
	__le16 csum_type;
	u8 root_level;
	u8 chunk_root_level;
	u8 log_root_level;
	struct btrfs_dev_item dev_item;
	char label[BTRFS_LABEL_SIZE];
	__le64 cache_generation;
	__le64 reserved[31];
	u8 sys_chunk_array[BTRFS_SYSTEM_CHUNK_ARRAY_SIZE];
	struct btrfs_root_backup super_roots[BTRFS_NUM_BACKUP_ROOTS];
} __attribute__ ((__packed__));

static inline u64 btrfs_sb_offset(int mirror)
{
	u64 start = 16 * 1024;
	if (mirror)
		return start << (BTRFS_SUPER_MIRROR_SHIFT * mirror);
	return BTRFS_SUPER_INFO_OFFSET;
}

#endif
