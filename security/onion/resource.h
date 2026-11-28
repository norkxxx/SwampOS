#pragma once
#include "../../kernel/session.h"
#include "../../kernel/ext4.h"
#include "../../filesystem/ramfs/ramfs.h"
struct resource_request {char app[128], path[128]; unsigned ram;};
struct resource_snapshot {size_t length;unsigned char bytes[SESSION_DATA_MAX];};
int resource_parse(const char *, struct resource_request *);
void resource_clear(struct resource_snapshot *);
int resource_prepare(const struct resource_request *,const struct ramfs *,const struct ext4 *,struct resource_snapshot *);
