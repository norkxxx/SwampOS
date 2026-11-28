#pragma once
#include "controller.h"
/* BIOS/OS legacy semaphore synchronization. Platform must grant exclusive
 * access to validated MMIO resource; no PCI or DMA activation here. Byte-only
 * OS semaphore write avoids racing a concurrent BIOS semaphore release. */
typedef int (*xhci_write8_fn)(void*,uint32_t,uint8_t);
/* Returns 1 if absent or firmware ownership safely released and SMI enables
 * are already clear. On timeout leaves OS request asserted; no forced takeover. */
int xhci_legacy_claim(const struct xhci_io*,xhci_write8_fn);
