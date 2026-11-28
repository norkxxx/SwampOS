#pragma once
#include "../../kernel/bootinfo.h"
struct efi_mem {uint32_t type,pad;uint64_t physical,virtual_address,pages,attributes;};
/* Only ordinary conventional RAM is passed to the kernel allocator. */
int handoff_map(struct swamp_bootinfo *,const void *,size_t,size_t);
