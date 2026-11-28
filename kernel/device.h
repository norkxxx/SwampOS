#pragma once
#include "pci.h"
enum device_kind {DEV_OTHER,DEV_AHCI,DEV_NVME,DEV_XHCI,DEV_ETHERNET,DEV_DISPLAY,DEV_VIRTIO,DEV_UHCI,DEV_OHCI,DEV_EHCI};
struct pci_bar {uint64_t address;uint8_t kind;}; /* 0 unused, 1 I/O, 2 mem32, 3 mem64 */
struct device_info {enum device_kind kind;struct pci_bar bars[6];uint8_t cap_count,pcie,msi,msix,virtio_vendor;};
const char *device_name(enum device_kind);
int device_probe(pci_read_fn,void*,const struct pci_function*,struct device_info*);
