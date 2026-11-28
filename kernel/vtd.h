#pragma once
#include "pmm.h"

/* Legacy Q35 domain: one BDF, up to seven sparse, non-identity DMA windows. */
#define USB_DMA_WINDOWS 7u
#define USB_IOVA(i) (((uint64_t)(i) + 1u) << 16)
int vtd_available(uint64_t registers);
int vtd_bind(unsigned bus,unsigned devfn,const struct dma_lease leases[USB_DMA_WINDOWS],unsigned count);
int vtd_revoke(void);
