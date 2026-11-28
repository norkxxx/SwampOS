#include "pci_resource.h"

#define EFI_PCI_IO_GUID \
    ((guid){0x4cf5b200,0x68b8,0x4ca5,{0x9e,0xec,0xb2,0x3e,0x3f,0x50,0x02,0x9a}})
#define PCI_CLASS_SERIAL 0x0cu
#define PCI_SUBCLASS_USB 0x03u
#define PCI_PROGIF_XHCI  0x30u
#define PCI_BAR_IO       0x01u
#define PCI_BAR_TYPE     0x06u
#define PCI_BAR_32       0x00u
#define PCI_BAR_64       0x04u
#define PCI_BAR_ADDRESS  0xfffffff0u
#define PCI_COMMAND_IO   0x0001u
#define PCI_COMMAND_MEM  0x0002u
#define PCI_COMMAND_BUS  0x0004u
#define EFI_PCI_WIDTH_UINT32 2u
#define EFI_RESOURCE_QWORD 0x8au
#define EFI_RESOURCE_END   0x79u
#define EFI_RESOURCE_MEMORY 0u
#define EFI_RESOURCE_LIMIT 4096u
#define XHCI_BAR_MAX (2u * 1024u * 1024u)

static uint16_t rd16(const uint8_t *p) {
    return (uint16_t)p[0] | (uint16_t)p[1] << 8;
}
static uint32_t rd32(const uint8_t *p) {
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 |
           (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}
static uint64_t rd64(const uint8_t *p) {
    return rd32(p) | (uint64_t)rd32(p + 4) << 32;
}

int efi_pci_bar0_resource(const void *buffer, size_t bytes,
                          uint64_t base, uint64_t *length_out) {
    if (!buffer || !length_out || !bytes || bytes > EFI_RESOURCE_LIMIT ||
        !base || (base & 4095u) || base >= (1ull << 52))
        return 0;

    const uint8_t *p = buffer;
    size_t off = 0;
    uint64_t found_length = 0;
    unsigned matches = 0;
    while (off < bytes) {
        uint8_t type = p[off];
        if (type == EFI_RESOURCE_END) {
            if (bytes - off < 2)
                return 0;
            if (p[off + 1] != 0) {
                uint8_t sum = 0;
                for (size_t i = 0; i < off + 2; i++)
                    sum = (uint8_t)(sum + p[i]);
                if (sum != 0)
                    return 0;
            }
            if (matches != 1)
                return 0;
            *length_out = found_length;
            return 1;
        }
        if (bytes - off < 3)
            return 0;
        uint16_t payload = rd16(p + off + 1);
        size_t total = (size_t)payload + 3;
        if (total > bytes - off)
            return 0;

        uint8_t resource_type = 0xff;
        uint64_t min = 0, max = 0, translation = 0, length = 0;
        if (type == EFI_RESOURCE_QWORD) {
            /* UEFI permits optional ResourceSourceIndex/ResourceSource bytes
             * after the 43-byte QWord address-space payload. */
            if (payload < 43)
                return 0;
            const uint8_t *d = p + off;
            resource_type = d[3];
            min = rd64(d + 14);
            max = rd64(d + 22);
            translation = rd64(d + 30);
            length = rd64(d + 38);
        } else if (type == 0x87u) { /* ACPI DWord Address Space Descriptor */
            /* DWord descriptor payload is 23 bytes before optional source. */
            if (payload < 23)
                return 0;
            const uint8_t *d = p + off;
            resource_type = d[3];
            min = rd32(d + 10);
            max = rd32(d + 14);
            translation = rd32(d + 18);
            length = rd32(d + 22);
        }
        if (resource_type == EFI_RESOURCE_MEMORY && length) {
            /* EDK II GetBarAttributes uses AddrRangeMax as an alignment
             * mask. Require an exact assigned BAR, natural alignment and the
             * reported address width before accepting that form. */
            if (min > max) {
                if (type != EFI_RESOURCE_QWORD || min != base ||
                    max != length - 1 || (length & max) || (base & max) ||
                    length > XHCI_BAR_MAX || length > (1ull << 52) - base)
                    return 0;
                uint64_t bits = rd64(p + off + 6);
                if (bits != 64 && (bits != 32 || base + length > (1ull << 32)))
                    return 0;
                max = base + length - 1;
            }
            if (base >= min && base <= max) {
                if (translation != 0 || length - 1 > max - base ||
                    length > XHCI_BAR_MAX || (length & 4095u) ||
                    length > (1ull << 52) - base)
                    return 0;
                found_length = length;
                matches++;
                if (matches > 1)
                    return 0;
            }
        }
        off += total;
    }
    return 0; /* The resource list must contain an explicit EndTag. */
}

static int pci_read32(struct efi_pci_io *io, uint32_t offset, uint32_t *value) {
    return io && io->pci.read && value && !(offset & 3u) && offset <= 0xfcu &&
           !EFI_ERROR(io->pci.read(io, EFI_PCI_WIDTH_UINT32, offset, 1, value));
}

static int already_seen(const struct swamp_bootinfo *b, uint16_t segment,
                        uint8_t bus, uint8_t device, uint8_t function) {
    for (unsigned i = 0; i < b->xhci_count; i++) {
        const struct swamp_xhci_resource *r = &b->xhci[i];
        if (r->segment == segment && r->bus == bus && r->device == device &&
            r->function == function)
            return 1;
    }
    return 0;
}

void efi_collect_xhci_resources(system_table *s, struct swamp_bootinfo *b) {
    if (!s || !s->bs || !s->bs->locate_handle_buffer ||
        !s->bs->handle_protocol || !s->bs->free_pool || !b)
        return;

    b->xhci_magic = SWAMP_XHCI_HANDOFF_MAGIC;
    b->xhci_count = 0;
    b->xhci_version = SWAMP_XHCI_HANDOFF_VERSION;
    handle *handles = 0;
    size_t count = 0;
    status st = s->bs->locate_handle_buffer(EFI_PCI_IO_BY_PROTOCOL,
                                             &EFI_PCI_IO_GUID, 0,
                                             &count, &handles);
    if (EFI_ERROR(st) || !handles || !count || count > 1024) {
        if (handles)
            s->bs->free_pool(handles);
        return;
    }

    for (size_t i = 0; i < count; i++) {
        struct efi_pci_io *io = 0;
        if (EFI_ERROR(s->bs->handle_protocol(handles[i], &EFI_PCI_IO_GUID,
                                              (void **)&io)) || !io ||
            !io->get_location || !io->get_bar_attributes)
            continue;

        size_t segment = 0, bus = 0, device = 0, function = 0;
        if (EFI_ERROR(io->get_location(io, &segment, &bus, &device, &function)) ||
            segment > 0xffffu || bus > 0xffu || device > 31 || function > 7)
            continue;

        uint32_t id, class_rev, header, command_status, bar_lo;
        if (!pci_read32(io, 0x00, &id) || !pci_read32(io, 0x08, &class_rev) ||
            !pci_read32(io, 0x0c, &header) || !pci_read32(io, 0x04, &command_status) )
            continue;
        unsigned class_id = class_rev >> 8;
        int fixture = segment == 0 && bus == 0 && device == 5 && function == 0 &&
            ((class_id == 0x010601 && id == 0x29228086) ||
             (class_id == 0x010802 && id == 0x00101b36));
        unsigned bar = fixture && class_id == 0x010601 ? 5 : 0;
        if (!fixture && (class_id != 0x0c0330 || b->xhci_count >= SWAMP_XHCI_RESOURCE_MAX))
            continue;
        if (!pci_read32(io, 0x10 + 4 * bar, &bar_lo))
            continue;
        if ((id & 0xffffu) == 0xffffu || !(id & 0xffffu) || (id >> 16) == 0xffffu ||
            (((header >> 16) & 0x7fu) != 0) || (bar_lo & PCI_BAR_IO))
            continue;

        uint64_t base;
        unsigned bar_kind;
        unsigned bar_type = bar_lo & PCI_BAR_TYPE;
        if (bar_type == PCI_BAR_32) {
            base = bar_lo & PCI_BAR_ADDRESS;
            bar_kind = 2;
        } else if (bar_type == PCI_BAR_64) {
            uint32_t high;
            if (bar == 5 || !pci_read32(io, 0x14 + 4 * bar, &high))
                continue;
            base = ((uint64_t)high << 32) | (bar_lo & PCI_BAR_ADDRESS);
            bar_kind = 3;
        } else {
            continue;
        }
        if (!base || (base & 4095u) || base >= (1ull << 52) ||
            already_seen(b, (uint16_t)segment, (uint8_t)bus,
                         (uint8_t)device, (uint8_t)function))
            continue;

        uint64_t supports = 0, length = 0;
        void *resources = 0;
        st = io->get_bar_attributes(io, bar, &supports, &resources);
        (void)supports;
        if (EFI_ERROR(st) || !resources) {
            if (resources)
                s->bs->free_pool(resources);
            continue;
        }
        int valid = efi_pci_bar0_resource(resources, EFI_RESOURCE_LIMIT,
                                           base, &length);
        s->bs->free_pool(resources);
        if (!valid)
            continue;

        struct swamp_xhci_resource *r = fixture ? &b->storage_resource : &b->xhci[b->xhci_count++];
        *r = (struct swamp_xhci_resource){
            .base = base,
            .length = length,
            .segment = (uint16_t)segment,
            .vendor = (uint16_t)id,
            .product = (uint16_t)(id >> 16),
            .command = (uint16_t)command_status,
            .bus = (uint8_t)bus,
            .device = (uint8_t)device,
            .function = (uint8_t)function,
            .bar = bar,
            .flags = (command_status & PCI_COMMAND_MEM ? 1u : 0u) |
                     (command_status & PCI_COMMAND_BUS ? 2u : 0u) |
                     (command_status & PCI_COMMAND_IO ? 4u : 0u) |
                     (bar_kind == 3 ? 8u : 0u),
        };
    }
    s->bs->free_pool(handles);
}
