#pragma once
#include "pmm.h"
#define USER_CODE 0x40000000ull
#define USER_GUARD 0x40001000ull
#define USER_STACK 0x40002000ull
#define USER_TOP (USER_STACK+4096)
#define USER_DATA 0x40004000ull
struct space {uint64_t root, pdpt, pd, pt, code, stack, data;};
extern struct pmm frames;
int vm_init(void);
int vm_create(struct space *,const void *,size_t);
int vm_create_data(struct space*,const void*,size_t,const void*,size_t,size_t);
void vm_destroy(struct space *);
void vm_activate(uint64_t);
uint64_t vm_kernel_root(void);
int user_readable(uint64_t,size_t);
int user_writable(uint64_t,size_t);
int vm_selftest(void);

int vm_map_live_fb(uint64_t physical,uint64_t bytes);

/* Low-memory identity-mapped, supervisor-only coherent CPU buffers. No IOMMU setup. */
int vm_dma_alloc(unsigned,unsigned,struct dma_lease*);
int vm_dma_release(struct dma_lease*,int quiesced);

#define VM_MMIO_VIRT 0x10000000000ull
#define VM_MMIO_LIMIT (2ull*1024*1024)
/* One permanent UC mapping, kernel root only; caller must independently verify
 * firmware/PCI resource ownership and absence of RAM aliases. No user mapping. */
int vm_map_mmio(uint64_t physical,uint64_t bytes);
int vm_mmio_matches(uint64_t physical,uint64_t bytes);
#ifdef VM_HOST
int vm_host_set_e820(const struct e820_entry *,size_t);
#endif

#define VM_IOMMU_VIRT (3ull << 39)
int vm_map_iommu(uint64_t physical);
