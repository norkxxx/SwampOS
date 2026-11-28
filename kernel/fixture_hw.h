#pragma once
#include "fixture_disk.h"
#include "vm.h"
#include "vtd.h"

#define FIXTURE_SERIAL "SWAMP38-DISPOSABLE"
enum { DMA_PAGES = 5, DATA_PAGE = 4 };
struct fixture_hw { uint8_t *dma; uint64_t iova; };
static inline volatile uint32_t *fixture_reg(unsigned offset) {
    return (volatile uint32_t *)(uintptr_t)(VM_MMIO_VIRT + offset);
}
static inline void dma_fence(void) { __asm__ volatile("mfence" ::: "memory"); }
static inline void clear_bytes(void *buf, unsigned length) {
    volatile uint8_t *p = buf;
    while (length--) *p++ = 0;
}
int fixture_wait(unsigned reg, uint32_t mask, uint32_t expected);
int ahci_idle(void);
int ahci_start(struct fixture_hw *);
int ahci_io(struct fixture_hw *, enum fixture_op, unsigned);
int nvme_idle(void);
int nvme_start(struct fixture_hw *, unsigned aperture);
int nvme_io(struct fixture_hw *, enum fixture_op, unsigned);
