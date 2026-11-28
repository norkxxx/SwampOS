#pragma once
#include <stdint.h>
#define GUARD_MAX 32u
struct guard_entry {uint8_t volume_id[16];uint32_t inode;uint64_t ceiling;};
struct guard_table {struct guard_entry entry[GUARD_MAX];};
/* Profiles are keyed by EXT4 volume UUID + inode, not inode alone. */
int guard_set(struct guard_table *,const uint8_t[16],uint32_t,uint64_t);
int guard_remove(struct guard_table *,const uint8_t[16],uint32_t);
uint64_t guard_ceiling(const struct guard_table *,const uint8_t[16],uint32_t);
int guard_allows(const struct guard_table *,const uint8_t[16],uint32_t,uint64_t);
int guard_parse_mask(const char *,uint64_t *);
