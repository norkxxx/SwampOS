#pragma once
#include "controller.h"
#include "../../../kernel/pci.h"
/* Independent firmware/resource manager must supply trusted assigned BAR base
 * and aperture length. Never probe BAR size by writing to PCI config space. */
struct xhci_pci_resource {uint64_t base;uint32_t bytes;unsigned memory_enabled,bus_master;};
int xhci_pci_resource(pci_read_fn,void*,unsigned,unsigned,unsigned,uint64_t,uint32_t,struct xhci_pci_resource*);
struct xhci_mmio_window {volatile uint32_t*base;uint32_t bytes;unsigned writable;};
int xhci_mmio_read32(void*,uint32_t,uint32_t*);
int xhci_mmio_write32(void*,uint32_t,uint32_t);

/* Use for the byte-wide legacy ownership semaphore only. */
int xhci_mmio_write8(void*,uint32_t,uint8_t);
