#include "pci_inventory.h"

#define PCI_COMMAND_IO       0x0001u
#define PCI_COMMAND_MEMORY   0x0002u
#define PCI_COMMAND_MASTER   0x0004u
#define PCI_BAR_IO           0x00000001u
#define PCI_BAR_TYPE_MASK    0x00000006u
#define PCI_BAR_TYPE_32      0x00000000u
#define PCI_BAR_TYPE_64      0x00000004u
#define PCI_BAR_ADDR_MASK    0xfffffff0u
#define XHCI_CLASS           0x0cu
#define XHCI_SUBCLASS        0x03u
#define XHCI_PROGIF          0x30u

static int read_reg(pci_read_fn read, void *ctx,
                    const struct pci_function *p, uint8_t reg,
                    uint32_t *value) {
    if (!read || !p || !value || (reg & 3u) || reg > 0xfcu)
        return 0;
    *value = read(ctx, p->bus, p->device, p->function, reg);
    return *value != UINT32_MAX;
}

int xhci_pci_inspect(pci_read_fn read, void *ctx,
                     const struct pci_function *p,
                     struct xhci_pci_info *out) {
    if (!read || !p || !out || p->device > 31 || p->function > 7 ||
        !p->vendor || p->vendor == 0xffffu || p->product == 0xffffu ||
        p->class_code != XHCI_CLASS || p->subclass != XHCI_SUBCLASS ||
        p->prog_if != XHCI_PROGIF || (p->header & 0x7fu) != 0)
        return 0;

    uint32_t id, class_rev, header, command_status, bar_lo;
    if (!read_reg(read, ctx, p, 0x00, &id) ||
        !read_reg(read, ctx, p, 0x08, &class_rev) ||
        !read_reg(read, ctx, p, 0x0c, &header) ||
        !read_reg(read, ctx, p, 0x04, &command_status) ||
        !read_reg(read, ctx, p, 0x10, &bar_lo))
        return 0;

    if (!(id & 0xffffu) || (id & 0xffffu) == 0xffffu ||
        (id >> 16) == 0xffffu ||
        (id & 0xffffu) != p->vendor || (id >> 16) != p->product ||
        (class_rev >> 24) != XHCI_CLASS ||
        ((class_rev >> 16) & 0xffu) != XHCI_SUBCLASS ||
        ((class_rev >> 8) & 0xffu) != XHCI_PROGIF ||
        (((header >> 16) & 0x7fu) != 0) ||
        (bar_lo & PCI_BAR_IO))
        return 0;

    unsigned type = bar_lo & PCI_BAR_TYPE_MASK;
    uint64_t base;
    uint8_t kind;
    if (type == PCI_BAR_TYPE_32) {
        base = bar_lo & PCI_BAR_ADDR_MASK;
        kind = 2;
    } else if (type == PCI_BAR_TYPE_64) {
        uint32_t bar_hi;
        if (!read_reg(read, ctx, p, 0x14, &bar_hi))
            return 0;
        base = ((uint64_t)bar_hi << 32) | (bar_lo & PCI_BAR_ADDR_MASK);
        kind = 3;
    } else {
        return 0;
    }

    /* An all-zero BAR is unassigned. A 64-bit all-ones value is invalid. */
    if (base == UINT64_MAX || (base && base >= (1ull << 52)))
        return 0;

    struct xhci_pci_info result = {
        .bus = p->bus,
        .device = p->device,
        .function = p->function,
        .vendor = (uint16_t)id,
        .product = (uint16_t)(id >> 16),
        .command = (uint16_t)command_status,
        .status = (uint16_t)(command_status >> 16),
        .bar0 = base,
        .bar_kind = kind,
        .bar_assigned = (uint8_t)(base != 0),
        .memory_enabled = (uint8_t)!!(command_status & PCI_COMMAND_MEMORY),
        .bus_master_enabled = (uint8_t)!!(command_status & PCI_COMMAND_MASTER),
        .io_enabled = (uint8_t)!!(command_status & PCI_COMMAND_IO),
    };
    *out = result;
    return 1;
}

int xhci_pci_resource_matches(pci_read_fn read, void *ctx,
                              const struct swamp_xhci_resource *r) {
    if (!r || r->segment != 0 || r->device > 31 || r->function > 7 ||
        r->bar != 0 || (r->flags & ~15u) || !r->base ||
        (r->base & 4095u) || !r->length || (r->length & 4095u) ||
        r->length > 2u * 1024u * 1024u || r->base >= (1ull << 52) ||
        r->length > (1ull << 52) - r->base)
        return 0;

    struct pci_function function = {
        .bus = r->bus, .device = r->device, .function = r->function,
        .class_code = 0x0c, .subclass = 0x03, .prog_if = 0x30,
        .header = 0, .vendor = r->vendor, .product = r->product
    };
    struct xhci_pci_info info;
    if (!xhci_pci_inspect(read, ctx, &function, &info) ||
        !info.bar_assigned || info.bar0 != r->base ||
        info.command != r->command)
        return 0;
    uint32_t flags = (info.memory_enabled ? 1u : 0u) |
                     (info.bus_master_enabled ? 2u : 0u) |
                     (info.io_enabled ? 4u : 0u) |
                     (info.bar_kind == 3 ? 8u : 0u);
    return flags == r->flags;
}
