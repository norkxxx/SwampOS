#pragma once
#include <stdint.h>
#include <stddef.h>
#define FRAME_SIZE 4096ull
#define PHYS_LIMIT (128ull*1024*1024)
#define FRAME_COUNT (PHYS_LIMIT/FRAME_SIZE)
struct e820_entry { uint64_t base,length; uint32_t type,attr; } __attribute__((packed));
struct pmm { uint64_t eligible[FRAME_COUNT/64], used[FRAME_COUNT/64]; unsigned total, free, hint; };
int pmm_init(struct pmm *,const struct e820_entry *,size_t,uint64_t);
uint64_t pmm_take(struct pmm *);
int pmm_release(struct pmm *,uint64_t);
