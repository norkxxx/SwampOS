#pragma once
#include <stddef.h>
#include <stdint.h>
struct blockdev {void *ctx; uint64_t sectors; int (*read)(void *,uint64_t,unsigned,void *);};
int block_read(const struct blockdev *,uint64_t,unsigned,void *);
int ata_secondary_master(struct blockdev *);
struct block_slice {struct blockdev parent;uint64_t first,count;};
int block_slice_init(struct block_slice *,struct blockdev *,const struct blockdev *,uint64_t,uint64_t);
