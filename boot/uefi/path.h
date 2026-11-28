#pragma once
#include <stddef.h>
#include <stdint.h>
/* GPT hard-drive node: type 4, subtype 1, length 42, signature at +24. */
struct disk_path {size_t hd,end;};
int parse_disk_path(const uint8_t*,size_t,struct disk_path*);
int same_disk(const uint8_t*,const struct disk_path*,const uint8_t*,const struct disk_path*);
int login_partition(const uint8_t*,const struct disk_path*);
size_t append_login_path(uint8_t*,size_t,const uint8_t*,const struct disk_path*);
