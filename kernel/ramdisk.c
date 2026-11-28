#include "ramdisk.h"
static int read_ram(void *ctx,uint64_t lba,unsigned n,void *dst){const struct ramdisk*r=ctx;if(!r||!dst||!n||lba>=r->bytes/512||n>r->bytes/512-lba)return 0;const unsigned char*p=r->data+lba*512;unsigned char*q=dst;size_t bytes=(size_t)n*512;while(bytes--)*q++=*p++;return 1;}
int ramdisk_bind(struct ramdisk*r,struct blockdev*d,const void*p,uint64_t n){if(!r||!d||!p||n!=8*1024*1024)return 0;*r=(struct ramdisk){p,n};*d=(struct blockdev){r,n/512,read_ram};return 1;}
int ramdisk_boot(struct ramdisk*r,struct blockdev*d,const struct swamp_bootinfo*b){if(!b||b->reserved!=1||b->ramdisk_size!=8*1024*1024||b->ramdisk<SWAMP_UEFI_RESERVE||b->ramdisk%4096||b->ramdisk>PHYS_LIMIT-b->ramdisk_size||!b->count||b->count>64)return 0;
 for(unsigned i=0;i<b->count;i++){const struct e820_entry*e=&b->map[i];if(e->length>UINT64_MAX-e->base)return 0;if(e->base<b->ramdisk+b->ramdisk_size&&b->ramdisk<e->base+e->length)return 0;}
 return ramdisk_bind(r,d,(const void*)(uintptr_t)b->ramdisk,b->ramdisk_size);}
