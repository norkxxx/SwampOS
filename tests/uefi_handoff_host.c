#include "../boot/uefi/handoff.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(void) {
    struct efi_mem map[] = {
        {7,0,0x00100000,0,0x1000,1},       /* low conventional RAM */
        {11,0,0xfebf0000,0,0x10,0},        /* PCI MMIO: excluded from RAM map */
        {7,0,0x100000000ull,0,0x10,1},     /* high conventional RAM */
        {10,0,0x200000000ull,0,0x10,0},    /* ACPI NVS: must not alias MMIO */
        {12,0,0xf0000000,0,0x10,0},        /* MMIO port-space */
        {7,0,0x00200000,0,0x100,1ull<<63} /* runtime RAM: physical range still recorded */
    };
    struct swamp_bootinfo b;
    memset(&b,0,sizeof b);
    assert(handoff_map(&b,map,sizeof map,sizeof map[0]));
    assert(b.count==1 && b.map[0].base==SWAMP_UEFI_RESERVE);
    assert(b.phys_complete==1 && b.phys_count==4);
    assert(b.phys_map[0].base==0x00100000 && b.phys_map[0].length==0x1000*4096ull);
    assert(b.phys_map[1].base==0x100000000ull && b.phys_map[1].length==0x10*4096ull);
    assert(b.phys_map[2].base==0x200000000ull && b.phys_map[2].length==0x10*4096ull);
    assert(b.phys_map[3].base==0x00200000 && b.phys_map[3].length==0x100*4096ull);

    /* More than 64 physical memory ranges disables MMIO mapping, not boot. */
    struct efi_mem many[66];
    memset(many,0,sizeof many);
    many[0]=(struct efi_mem){7,0,0x400000,0,0x100,1};
    for(unsigned i=1;i<66;i++)many[i]=(struct efi_mem){7,0,0x100000000ull+(uint64_t)i*0x20000,0,0x10,1};
    memset(&b,0,sizeof b);
    assert(handoff_map(&b,many,sizeof many,sizeof many[0]));
    assert(b.count==1 && b.phys_complete==0 && b.phys_count==0);

    many[2].pages=0;
    memset(&b,0,sizeof b);
    assert(!handoff_map(&b,many,sizeof many,sizeof many[0]));

    puts("UEFI_HANDOFF_HOST_OK: low PMM map, high RAM/NVS alias map, MMIO exclusion, runtime range capture, overflow fail-closed");
    return 0;
}
