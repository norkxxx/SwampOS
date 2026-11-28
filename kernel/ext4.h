#pragma once
#include "block.h"
#define FS_BLOCK 4096u
#define FS_MAX_FILE (64u*1024u)
struct ext4 {struct blockdev dev; uint32_t blocks,inodes,ipg,bpg,table; uint16_t isize; uint8_t uuid[16];};
struct fs_inode {uint32_t number,size;uint16_t mode;uint8_t extent[60];};
int ext4_mount(struct ext4 *,const struct blockdev *);
int ext4_inode(const struct ext4 *,uint32_t,struct fs_inode *);
int ext4_read(const struct ext4 *,const struct fs_inode *,uint32_t,void *,size_t);
int ext4_lookup(const struct ext4 *,const char *,struct fs_inode *);
typedef int (*fs_visit)(void *,const char *,size_t,uint32_t);
int ext4_list(const struct ext4 *,const struct fs_inode *,fs_visit,void *);
