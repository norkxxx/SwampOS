#pragma once
#include "../../../kernel/pci.h"
#include "../../../kernel/bootinfo.h"

/* Read-only xHCI PCI function inventory. This does not size BARs, write PCI
 * configuration space, enable memory decoding/bus mastering, or touch MMIO. */
struct xhci_pci_info {
    uint8_t bus, device, function;
    uint16_t vendor, product, command, status;
    uint64_t bar0;
    uint8_t bar_kind;       /* 0 unknown, 2 memory32, 3 memory64 */
    uint8_t bar_assigned;
    uint8_t memory_enabled;
    uint8_t bus_master_enabled;
    uint8_t io_enabled;
};

int xhci_pci_inspect(pci_read_fn read, void *ctx,
                     const struct pci_function *function,
                     struct xhci_pci_info *out);

/* Revalidate a UEFI-reported BAR0 record against live PCI config space. */
int xhci_pci_resource_matches(pci_read_fn read, void *ctx,
                              const struct swamp_xhci_resource *resource);
