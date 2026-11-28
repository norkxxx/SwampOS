#include "fixture_hw.h"
#include "bootinfo.h"
#include "hardware.h"
#include "core.h"
#include "fwcfg.h"
#include "pci.h"

static struct dma_lease lease[USB_DMA_WINDOWS];
static struct fixture_hw hw;
static unsigned command;
static int active, nvme, quarantined;

int fixture_wait(unsigned reg, uint32_t mask, uint32_t expected) {
    uint64_t begin = ticks();
    do {
        if ((*fixture_reg(reg) & mask) == expected) return 1;
        __asm__ volatile("pause");
    } while (ticks() - begin < 150);
    return 0;
}

static int bus_master(int on) {
    uint32_t address = 0x80002804; /* segment zero, 00:05.0, PCI command */
    uint16_t value = (command & ~4u) | (on ? 4u : 0u);
    uint64_t flags;
    __asm__ volatile("pushfq; pop %0; cli" : "=r"(flags) :: "memory");
    __asm__ volatile("outl %0,%1" :: "a"(address), "Nd"((uint16_t)0xcf8));
    __asm__ volatile("outw %0,%1" :: "a"(value), "Nd"((uint16_t)0xcfc));
    if (flags & 0x200) __asm__ volatile("sti" ::: "memory");
    return (uint16_t)pci_config_read(0,0,5,0,4) == value;
}

const char *fixture_open(void) {
    if (active) return 0;
    if (quarantined) return "controller quarantined until reboot";
    if (!q35_live_option("opt/swamp/disposable-storage")) return "Q35 Live opt-in required";
    const struct swamp_bootinfo *boot = (const void *)SWAMP_UEFI_INFO;
    const struct swamp_xhci_resource *r = &boot->storage_resource;
    uint32_t id = pci_config_read(0,0,5,0,0), pci_class = pci_config_read(0,0,5,0,8) >> 8;
    nvme = id == 0x00101b36 && pci_class == 0x010802;
    if (!nvme && !(id == 0x29228086 && pci_class == 0x010601)) return "unsupported controller at 00:05.0";
    unsigned bar = nvme ? 0 : 5;
    uint32_t lo = pci_config_read(0,0,5,0,0x10 + 4*bar);
    uint64_t base = lo & ~15u;
    if ((lo & 7) == 4 && nvme) base |= (uint64_t)pci_config_read(0,0,5,0,0x14) << 32;
    else if (lo & 7) return "invalid PCI BAR";
    command = (uint16_t)pci_config_read(0,0,5,0,4);
    if (!boot->phys_complete || r->segment || r->bus || r->device != 5 || r->function ||
        r->bar != bar || r->vendor != (uint16_t)id || r->product != id>>16 ||
        !(r->command & command & 2) || base != r->base || r->length < (nvme ? 8192u : 4096u))
        return "firmware aperture mismatch";
    struct acpi_info acpi;
    if (!acpi_get_info(&acpi) || acpi.dmar_drhd_count != 1 || acpi.dmar_rmrr_count ||
        !acpi_dmar_covers(&acpi,0,0,5,0) || !vtd_available(acpi.drhd[0].reg_base))
        return "VT-d unavailable or in use";
    if (!vm_map_mmio(base,r->length)) return "controller aperture unavailable";
    if (!vm_dma_alloc(DMA_PAGES,1,&lease[0])) return "DMA allocation failed";
    hw = (struct fixture_hw){(void *)(uintptr_t)lease[0].base, USB_IOVA(0)};
    quarantined = 1;
    if (!bus_master(0) || !(nvme ? nvme_idle() : ahci_idle())) return "controller did not quiesce";
    if (!vtd_bind(0,5 << 3,lease,1)) return "VT-d bind failed";
    active = 1;
    if (!bus_master(1) || !(nvme ? nvme_start(&hw,r->length) : ahci_start(&hw))) {
        fixture_close();
        return "identify or queue setup failed";
    }
    quarantined = 0;
    return 0;
}

void fixture_close(void) {
    if (!active) return;
    active = 0;
    int stopped = nvme ? nvme_idle() : ahci_idle();
    int blocked = bus_master(0);
    int revoked = vtd_revoke();
    quarantined = !(stopped && blocked && revoked && vm_dma_release(&lease[0],1));
}

int fixture_io(enum fixture_op op, unsigned lba, void *sector) {
    if (!active || lba > 2 || (op != FIXTURE_FLUSH && !sector) ||
        (op == FIXTURE_WRITE && lba == 0)) return 0;
    uint8_t *data = hw.dma + DATA_PAGE*4096, *bytes = sector;
    if (op == FIXTURE_WRITE) for (unsigned i=0;i<FIXTURE_BLOCK;i++) data[i]=bytes[i];
    int ok = nvme ? nvme_io(&hw,op,lba) : ahci_io(&hw,op,lba);
    if (!ok) { fixture_close(); return 0; }
    if (op == FIXTURE_READ) for (unsigned i=0;i<FIXTURE_BLOCK;i++) bytes[i]=data[i];
    return 1;
}
