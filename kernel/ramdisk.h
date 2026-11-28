#pragma once
#include "block.h"
#include "bootinfo.h"
struct ramdisk {const unsigned char *data;uint64_t bytes;};
int ramdisk_bind(struct ramdisk*,struct blockdev*,const void*,uint64_t);
int ramdisk_boot(struct ramdisk*,struct blockdev*,const struct swamp_bootinfo*);
