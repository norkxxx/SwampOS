#include "../boot/uefi/pci_resource.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static void wr16(unsigned char *p, unsigned v) {
    p[0] = (unsigned char)v; p[1] = (unsigned char)(v >> 8);
}
static void wr64(unsigned char *p, uint64_t v) {
    for (unsigned i = 0; i < 8; i++) p[i] = (unsigned char)(v >> (8 * i));
}

static size_t descriptor_dword(unsigned char *p, uint32_t min,
                               uint32_t max, uint32_t translation,
                               uint32_t length) {
    memset(p, 0, 64);
    p[0] = 0x87; wr16(p + 1, 23); p[3] = 0;
    p[4] = 0; p[5] = 0;
    /* Granularity at +6; DWord fields begin at +10. */
    p[10]=(uint8_t)min; p[11]=(uint8_t)(min>>8); p[12]=(uint8_t)(min>>16); p[13]=(uint8_t)(min>>24);
    p[14]=(uint8_t)max; p[15]=(uint8_t)(max>>8); p[16]=(uint8_t)(max>>16); p[17]=(uint8_t)(max>>24);
    p[18]=(uint8_t)translation; p[19]=(uint8_t)(translation>>8); p[20]=(uint8_t)(translation>>16); p[21]=(uint8_t)(translation>>24);
    p[22]=(uint8_t)length; p[23]=(uint8_t)(length>>8); p[24]=(uint8_t)(length>>16); p[25]=(uint8_t)(length>>24);
    p[26]=0x79; p[27]=0;
    return 28;
}

static size_t descriptor(unsigned char *p, uint8_t resource_type,
                         uint64_t min, uint64_t max, uint64_t translation,
                         uint64_t length) {
    memset(p, 0, 64);
    p[0] = 0x8a; wr16(p + 1, 43); p[3] = resource_type;
    wr64(p + 6, 0); wr64(p + 14, min); wr64(p + 22, max);
    wr64(p + 30, translation); wr64(p + 38, length);
    p[46] = 0x79; p[47] = 0;
    return 48;
}

static void run_collector_test(void);

int main(void) {
    unsigned char b[128];
    uint64_t length = 0;
    const uint64_t base = 0xfebf0000ull;

    size_t n = descriptor(b, 0, base, base + 0xffff, 0, 0x10000);
    assert(efi_pci_bar0_resource(b, n, base, &length));
    assert(length == 0x10000);

    n = descriptor(b, 0, base, base + 0x1ffff, 0, 0x20000);
    assert(efi_pci_bar0_resource(b, n, base, &length) && length == 0x20000);

    /* EDK II QWord descriptor for QEMU's 64-bit BAR. */
    const uint64_t high = 0xc000000000ull;
    n = descriptor(b, 0, high, 0x3fff, 0, 0x4000);
    wr64(b + 6, 64);
    assert(efi_pci_bar0_resource(b, n, high, &length) && length == 0x4000);
    assert(!efi_pci_bar0_resource(b, n, high + 4096, &length));
    wr64(b + 6, 32);
    assert(!efi_pci_bar0_resource(b, n, high, &length));
    wr64(b + 6, 64); wr64(b + 22, 0x2fff);
    assert(!efi_pci_bar0_resource(b, n, high, &length));
    wr64(b + 22, 0x3fff); wr64(b + 30, 4096);
    assert(!efi_pci_bar0_resource(b, n, high, &length));

    n = descriptor_dword(b, 0xfebf0000u, 0xfebfffffu, 0, 0x10000);
    assert(efi_pci_bar0_resource(b, n, base, &length) && length == 0x10000);

    n = descriptor_dword(b, 0xfebf0000u, 0xfebfffffu, 0x1000, 0x10000);
    assert(!efi_pci_bar0_resource(b, n, base, &length)); /* DWord translated */

    n = descriptor(b, 1, base, base + 0xffff, 0, 0x10000);
    assert(!efi_pci_bar0_resource(b, n, base, &length)); /* I/O resource */
    n = descriptor(b, 0, base, base + 0xffff, 0x1000, 0x10000);
    assert(!efi_pci_bar0_resource(b, n, base, &length)); /* translated */
    n = descriptor(b, 0, base + 0x1000, base + 0x10fff, 0, 0x10000);
    assert(!efi_pci_bar0_resource(b, n, base, &length)); /* base outside */
    n = descriptor(b, 0, base, base + 0xffff, 0, 0x18000);
    assert(!efi_pci_bar0_resource(b, n, base, &length)); /* exceeds max */
    n = descriptor(b, 0, base, base + 0x2fffff, 0, 0x300000);
    assert(!efi_pci_bar0_resource(b, n, base, &length)); /* exceeds 2 MiB mapper limit */

    n = descriptor(b, 0, base, base + 0xffff, 0, 0x10000);
    /* Optional ResourceSourceIndex + short source string after QWord payload. */
    b[1] = 45; b[46] = 0x11; b[47] = 0x22; b[48] = 0x79; b[49] = 0;
    assert(efi_pci_bar0_resource(b, 50, base, &length) && length == 0x10000);

    n = descriptor(b, 0, base, base + 0xffff, 0, 0x10000);
    assert(!efi_pci_bar0_resource(b, n - 2, base, &length)); /* missing EndTag */
    b[1] = 42;
    assert(!efi_pci_bar0_resource(b, n, base, &length)); /* bad descriptor length */
    n = descriptor(b, 0, base, base + 0xffff, 0, 0x10000);
    b[47] = 1; /* nonzero EndTag checksum must validate */
    assert(!efi_pci_bar0_resource(b, n, base, &length));
    n = descriptor(b, 0, base, base + 0xffff, 0, 0x10000);
    b[0] = 0x88;
    assert(!efi_pci_bar0_resource(b, n, base, &length)); /* wrong descriptor form */
    n = descriptor(b, 0, base, base + 0xffff, 0, 0x10000);
    b[46] = 0;
    assert(!efi_pci_bar0_resource(b, n, base, &length)); /* no EndTag */
    n = descriptor(b, 0, base, base + 0xffff, 0, 0x10000);
    assert(!efi_pci_bar0_resource(b, n, 0, &length));
    assert(!efi_pci_bar0_resource(b, n, base + 1, &length));
    assert(!efi_pci_bar0_resource(b, n, 1ull << 52, &length));
    assert(!efi_pci_bar0_resource(b, n, base, 0));

    /* Duplicate matching QWord memory resources are ambiguous and rejected. */
    n = descriptor(b, 0, base, base + 0xffff, 0, 0x10000);
    unsigned char second[48];
    descriptor(second, 0, base, base + 0xffff, 0, 0x10000);
    memcpy(b + n - 2, second, 48);
    b[n - 2 + 46] = 0x79;
    b[n - 2 + 47] = 0;
    assert(!efi_pci_bar0_resource(b, n + 46, base, &length));

    run_collector_test();
    puts("UEFI_PCI_RESOURCE_HOST_OK: UEFI PCI I/O collection, 32/64-bit BAR0, QWord/DWord resource parsing, bounds, translation rejection and fail-closed protocol handling");
    return 0;
}

/* Exercise the actual firmware collector through a mock UEFI protocol table. */
static struct efi_pci_io mock_io;
static unsigned char mock_resources[64];
static unsigned mock_frees, mock_reads, mock_bar_calls;
static size_t mock_segment, mock_bus, mock_device, mock_function;
static uint32_t mock_regs[8];
static handle mock_handles[1] = {(handle)1};

static status EFIAPI mock_pci_read(struct efi_pci_io *io, uint32_t width,
                                    uint32_t offset, size_t count, void *out) {
    (void)io;
    if (width != 2 || count != 1 || (offset & 3) || offset > 0x14)
        return INVALID_PARAMETER;
    *(uint32_t *)out = mock_regs[offset / 4];
    mock_reads++;
    return SUCCESS;
}
static status EFIAPI mock_get_location(struct efi_pci_io *io, size_t *seg,
                                        size_t *bus, size_t *dev, size_t *fn) {
    (void)io; *seg=mock_segment; *bus=mock_bus; *dev=mock_device; *fn=mock_function;
    return SUCCESS;
}
static status EFIAPI mock_get_bar(struct efi_pci_io *io, uint8_t bar,
                                   uint64_t *supports, void **resources) {
    (void)io; mock_bar_calls++;
    if (bar != 0) return INVALID_PARAMETER;
    *supports = 0;
    *resources = mock_resources;
    return SUCCESS;
}
static status EFIAPI mock_locate(uint32_t search, const guid *protocol,
                                  void *key, size_t *count, handle **handles) {
    (void)protocol; (void)key;
    if (search != EFI_PCI_IO_BY_PROTOCOL) return INVALID_PARAMETER;
    *count = 1; *handles = mock_handles;
    return SUCCESS;
}
static status EFIAPI mock_handle_protocol(handle h, const guid *protocol,
                                           void **interface) {
    (void)protocol;
    if (h != mock_handles[0]) return NOT_FOUND;
    *interface = &mock_io;
    return SUCCESS;
}
static status EFIAPI mock_free_pool(void *p) {
    (void)p; mock_frees++; return SUCCESS;
}

static void run_collector_test(void) {
    memset(&mock_io, 0, sizeof mock_io);
    memset(mock_regs, 0, sizeof mock_regs);
    memset(mock_resources, 0, sizeof mock_resources);
    mock_reads = mock_frees = mock_bar_calls = 0;
    mock_segment=0; mock_bus=0; mock_device=20; mock_function=0;
    mock_regs[0]=0x1e318086u;
    mock_regs[1]=0x00000002u; /* MEM enabled, Bus Master disabled */
    mock_regs[2]=0x0c033000u;
    mock_regs[3]=0;
    mock_regs[4]=0xfebf0000u;
    mock_io.pci.read=mock_pci_read;
    mock_io.get_location=mock_get_location;
    mock_io.get_bar_attributes=mock_get_bar;
    descriptor(mock_resources,0,0xfebf0000ull,0xfebfffffull,0,0x10000);

    boot bs={0};
    bs.locate_handle_buffer=mock_locate;
    bs.handle_protocol=mock_handle_protocol;
    bs.free_pool=mock_free_pool;
    system_table st={0}; st.bs=&bs;
    struct swamp_bootinfo bi={0};
    efi_collect_xhci_resources(&st,&bi);
    assert(bi.xhci_magic==SWAMP_XHCI_HANDOFF_MAGIC && bi.xhci_version==SWAMP_XHCI_HANDOFF_VERSION && bi.xhci_count==1);
    assert(bi.xhci[0].base==0xfebf0000ull && bi.xhci[0].length==0x10000);
    assert(bi.xhci[0].vendor==0x8086 && bi.xhci[0].product==0x1e31);
    assert(bi.xhci[0].bus==0 && bi.xhci[0].device==20 && bi.xhci[0].bar==0);
    assert((bi.xhci[0].flags&1u)!=0 && (bi.xhci[0].flags&2u)==0);
    assert(mock_reads==5 && mock_bar_calls==1 && mock_frees==2);

    /* 64-bit BAR0 must combine the low and high configuration dwords. */
    mock_reads=mock_frees=mock_bar_calls=0;
    mock_regs[4]=0x00000004u; mock_regs[5]=1u;
    descriptor(mock_resources,0,0x100000000ull,0x10000ffffull,0,0x10000);
    memset(&bi,0,sizeof bi);
    efi_collect_xhci_resources(&st,&bi);
    assert(bi.xhci_count==1 && bi.xhci[0].base==0x100000000ull);
    assert(bi.xhci[0].length==0x10000 && (bi.xhci[0].flags&8u));
    assert(mock_reads==6 && mock_bar_calls==1 && mock_frees==2);

    /* The collector fails closed when the resource list cannot reconcile BAR0. */
    mock_frees=0; mock_bar_calls=0; mock_regs[2]=0x0c032000u;
    memset(&bi,0,sizeof bi);
    efi_collect_xhci_resources(&st,&bi);
    assert(bi.xhci_magic==SWAMP_XHCI_HANDOFF_MAGIC && bi.xhci_version==SWAMP_XHCI_HANDOFF_VERSION && bi.xhci_count==0);
    assert(mock_bar_calls==0 && mock_frees==1);
}
