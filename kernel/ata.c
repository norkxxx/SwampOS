#include "block.h"
#include "bootinfo.h"
#include "../drivers/storage/ata/pio.h"
/* Read-only secondary legacy IDE master. Live mode never calls this backend. */
static void out8(void *ctx,uint16_t p,uint8_t v){(void)ctx;__asm__ volatile("outb %0,%1"::"a"(v),"Nd"(p));}
static uint8_t in8(void *ctx,uint16_t p){(void)ctx;uint8_t v;__asm__ volatile("inb %1,%0":"=a"(v):"Nd"(p));return v;}
static uint16_t in16(void *ctx,uint16_t p){(void)ctx;uint16_t v;__asm__ volatile("inw %1,%0":"=a"(v):"Nd"(p));return v;}
int ata_secondary_master(struct blockdev *d){
    static struct ata_drive drive;
    if (!d) return 0;
    if (boot_uefi && ((const struct swamp_bootinfo*)SWAMP_UEFI_INFO)->reserved==1) { *d=(struct blockdev){0}; return 0; }
    return ata_pio_probe(&drive,d,(struct ata_io){0,in8,in16,out8},0x170,0x376,0);
}
