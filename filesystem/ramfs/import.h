#pragma once
#include "ramfs.h"
#include "../../kernel/ext4.h"
/* Explicit copy of a regular, bounded file from a read-only EXT4 mount. */
int ramfs_import_ext4(struct ramfs*,const struct ext4*,const char*,const char*,void*,size_t);
