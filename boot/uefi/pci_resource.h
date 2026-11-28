#pragma once
#include "efi.h"
#include "../../kernel/bootinfo.h"

#define EFI_PCI_IO_BY_PROTOCOL 2u

struct efi_pci_io;
typedef status (EFIAPI *efi_pci_config_read)(struct efi_pci_io *, uint32_t,
                                              uint32_t, size_t, void *);
typedef status (EFIAPI *efi_pci_get_location)(struct efi_pci_io *, size_t *,
                                               size_t *, size_t *, size_t *);
typedef status (EFIAPI *efi_pci_get_bar_attributes)(struct efi_pci_io *,
                                                     uint8_t, uint64_t *, void **);
struct efi_pci_io_access { void *read; void *write; };
struct efi_pci_config_access { efi_pci_config_read read; void *write; };
struct efi_pci_io {
    void *poll_mem;
    void *poll_io;
    struct efi_pci_io_access mem;
    struct efi_pci_io_access io;
    struct efi_pci_config_access pci;
    void *copy_mem;
    void *map;
    void *unmap;
    void *allocate_buffer;
    void *free_buffer;
    void *flush;
    efi_pci_get_location get_location;
    void *attributes;
    efi_pci_get_bar_attributes get_bar_attributes;
    void *set_bar_attributes;
    uint64_t rom_size;
    void *rom_image;
};
_Static_assert(offsetof(struct efi_pci_io, pci)==48,"EFI PCI I/O Pci member offset");
_Static_assert(offsetof(struct efi_pci_io, get_location)==112,"EFI PCI I/O GetLocation offset");
_Static_assert(offsetof(struct efi_pci_io, get_bar_attributes)==128,"EFI PCI I/O GetBarAttributes offset");
_Static_assert(sizeof(struct efi_pci_io)==160,"EFI PCI I/O protocol size");

/* Parse the bounded ACPI resource descriptor list returned by
 * EFI_PCI_IO_PROTOCOL.GetBarAttributes. Accept only a single, zero-translation
 * QWord or DWord memory descriptor which contains the currently assigned BAR base. */
int efi_pci_bar0_resource(const void *descriptors, size_t bytes,
                          uint64_t bar_base, uint64_t *bar_length);

/* Collect firmware-reported xHCI BAR0 resources. Read-only: this routine does
 * not change PCI attributes, BARs, bus mastering, MMIO or DMA state. */
void efi_collect_xhci_resources(system_table *system,
                                struct swamp_bootinfo *bootinfo);
