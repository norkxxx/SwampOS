#include "../drivers/usb/xhci/pci_inventory.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

struct mock_pci {
    uint32_t regs[64];
    unsigned reads;
    unsigned wrong_bdf;
};

static uint32_t read_cfg(void *ctx, uint8_t bus, uint8_t dev,
                         uint8_t fn, uint8_t reg) {
    struct mock_pci *m = ctx;
    m->reads++;
    if (bus != 2 || dev != 20 || fn != 0 || (reg & 3)) {
        m->wrong_bdf++;
        return UINT32_MAX;
    }
    return m->regs[reg / 4];
}

static struct pci_function function(void) {
    return (struct pci_function){
        .bus = 2, .device = 20, .function = 0,
        .class_code = 0x0c, .subclass = 0x03, .prog_if = 0x30,
        .header = 0, .vendor = 0x8086, .product = 0x1e31
    };
}

static void init(struct mock_pci *m) {
    memset(m, 0, sizeof *m);
    m->regs[0] = 0x1e318086u;
    m->regs[2] = 0x0c033000u;
    m->regs[3] = 0;
    m->regs[1] = (1u << 1) | (1u << 20); /* MEM decode + capability list */
    m->regs[4] = 0xfebf0000u;             /* 32-bit memory BAR */
}

int main(void) {
    struct mock_pci m;
    struct xhci_pci_info out;
    struct pci_function p = function();

    init(&m);
    assert(xhci_pci_inspect(read_cfg, &m, &p, &out));
    assert(out.bus == 2 && out.device == 20 && out.function == 0);
    assert(out.vendor == 0x8086 && out.product == 0x1e31);
    assert(out.bar_kind == 2 && out.bar_assigned && out.bar0 == 0xfebf0000u);
    assert(out.memory_enabled && !out.bus_master_enabled && !out.io_enabled);
    assert(out.command == (1u << 1) && out.status == (1u << 4));
    assert(m.wrong_bdf == 0);
    struct swamp_xhci_resource handoff = {
        .base=0xfebf0000ull, .length=0x10000, .segment=0,
        .vendor=0x8086, .product=0x1e31, .command=2,
        .bus=2, .device=20, .function=0, .bar=0, .flags=1
    };
    assert(xhci_pci_resource_matches(read_cfg, &m, &handoff));
    handoff.base += 0x1000;
    assert(!xhci_pci_resource_matches(read_cfg, &m, &handoff));
    handoff.base -= 0x1000; handoff.command |= 4;
    assert(!xhci_pci_resource_matches(read_cfg, &m, &handoff));
    handoff.command = 2; handoff.segment = 1;
    assert(!xhci_pci_resource_matches(read_cfg, &m, &handoff));

    init(&m);
    m.regs[4] = 0x00000004u; /* 64-bit BAR */
    m.regs[5] = 0x00000001u;
    assert(xhci_pci_inspect(read_cfg, &m, &p, &out));
    assert(out.bar_kind == 3 && out.bar_assigned && out.bar0 == 0x100000000ull);
    struct swamp_xhci_resource handoff64 = {
        .base=0x100000000ull, .length=0x10000, .segment=0,
        .vendor=0x8086, .product=0x1e31, .command=2,
        .bus=2, .device=20, .function=0, .bar=0, .flags=9
    };
    assert(xhci_pci_resource_matches(read_cfg, &m, &handoff64));

    init(&m);
    m.regs[1] = (1u << 1) | (1u << 2);
    assert(xhci_pci_inspect(read_cfg, &m, &p, &out));
    assert(out.memory_enabled && out.bus_master_enabled);

    init(&m);
    m.regs[4] = 0;
    assert(xhci_pci_inspect(read_cfg, &m, &p, &out));
    assert(!out.bar_assigned && out.bar0 == 0);

    init(&m); m.regs[4] |= 1u;
    assert(!xhci_pci_inspect(read_cfg, &m, &p, &out)); /* I/O BAR */
    init(&m); m.regs[4] = 0x00000006u;
    assert(!xhci_pci_inspect(read_cfg, &m, &p, &out)); /* reserved BAR type */
    init(&m); m.regs[4] = 0x00000004u; m.regs[5] = 0xffffffffu;
    assert(!xhci_pci_inspect(read_cfg, &m, &p, &out)); /* outside physical width */
    init(&m); p.prog_if = 0x20;
    assert(!xhci_pci_inspect(read_cfg, &m, &p, &out));
    p = function(); init(&m); m.regs[2] = 0x0c032000u;
    assert(!xhci_pci_inspect(read_cfg, &m, &p, &out));
    p = function(); init(&m); m.regs[0] ^= 1u;
    assert(!xhci_pci_inspect(read_cfg, &m, &p, &out));
    p = function(); init(&m); p.vendor = 0;
    assert(!xhci_pci_inspect(read_cfg, &m, &p, &out));
    p = function(); init(&m); m.regs[3] = 1u << 16; /* non-endpoint header */
    assert(!xhci_pci_inspect(read_cfg, &m, &p, &out));

    puts("XHCI_PCI_INVENTORY_HOST_OK: 32/64-bit BAR decode, command bits, unassigned BAR, class/BDF/header validation, invalid BAR rejection; read-only");
    return 0;
}
