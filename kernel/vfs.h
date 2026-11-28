#pragma once
#include "ext4.h"
#include "gpt.h"
/* Single read-only mount for lab Linux filesystem GUID only. Never opens vault partitions. */
struct vfs {struct block_slice slice;struct ext4 fs;uint8_t volume_id[16];unsigned mounted;};
int vfs_mount_demo(struct vfs*,const struct blockdev*);
int vfs_mount_partition(struct vfs*,const struct blockdev*,const struct gpt*,unsigned);
void vfs_unmount(struct vfs*);
