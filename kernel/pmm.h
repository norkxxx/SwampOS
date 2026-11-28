#pragma once
#include <stdint.h>
#include <stddef.h>
#define FRAME_SIZE 4096ull
#define PHYS_LIMIT (128ull*1024*1024)
#define FRAME_COUNT (PHYS_LIMIT/FRAME_SIZE)
struct e820_entry { uint64_t base,length; uint32_t type,attr; } __attribute__((packed));
#define DMA_LEASE_MAX 16u
#define DMA_LEASE_PAGES_MAX 64u
struct dma_lease {uint64_t base, token; unsigned pages;};
struct pmm { uint64_t eligible[FRAME_COUNT/64], used[FRAME_COUNT/64], pinned[FRAME_COUNT/64]; unsigned total, free, hint; struct dma_lease dma[DMA_LEASE_MAX]; uint64_t next_token; };
int pmm_init(struct pmm *,const struct e820_entry *,size_t,uint64_t);
uint64_t pmm_take(struct pmm *);
int pmm_release(struct pmm *,uint64_t);

/* Single-CPU kernel-only API. Pinned contiguous memory remains allocated until
 * a matching lease is released after independently confirmed DMA quiescence. */
int pmm_dma_take(struct pmm*,unsigned pages,unsigned align_pages,struct dma_lease*);
int pmm_dma_valid(const struct pmm*,const struct dma_lease*);
int pmm_dma_release(struct pmm*,const struct dma_lease*,int quiesced);
