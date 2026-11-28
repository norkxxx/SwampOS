/* Independent unsigned plaintext lab only. Never called by SwampLogin. */
#include "efi.h"
#include "handoff.h"
extern const unsigned char kernel_payload[],kernel_payload_end[];
#ifdef FARKUAD_LIVE
extern const unsigned char live_volume_start[],live_volume_end[];
#endif
extern void EFIAPI lab_jump(void) __attribute__((noreturn));
static void (EFIAPI *volatile transition)(void)=lab_jump;
static const guid acpi2={0x8868e871,0xe4f1,0x11d3,{0xbc,0x22,0x00,0x80,0xc7,0x3c,0x88,0x81}};
static const guid acpi1={0xeb9d2d30,0x2d88,0x11d3,{0x9a,0x16,0x00,0x90,0x27,0x3f,0xc1,0x4d}};
static const guid gop_guid={0x9042a9de,0x23dc,0x4a38,{0x96,0xfb,0x7a,0xde,0xd0,0x80,0x51,0x6a}};
struct config {guid id;void *table;};
struct gop_info {uint32_t version,width,height,format,masks[4],stride;};
struct gop_mode {uint32_t max,mode;struct gop_info *info;size_t size;uint64_t fb;size_t fbsize;};
struct gop {void *query,*set,*blt;struct gop_mode *mode;};
static int same(const guid*a,const guid*b){const unsigned char*x=(const void*)a,*y=(const void*)b;for(unsigned i=0;i<16;i++)if(x[i]!=y[i])return 0;return 1;}
static void zero(void *a,size_t n){volatile unsigned char *p=a;while(n--)*p++=0;}
#ifdef FARKUAD_LIVE
#include "live_menu.h"
#include "pci_resource.h"
#endif
status EFIAPI efi_main(handle self,system_table*s){
#ifdef FARKUAD_LIVE
 say(s,S("FARKUAD LIVE / UNSIGNED / VOLATILE\r\n"));
#else
 say(s,S("SWAMP KERNEL LAB / PLAINTEXT ONLY\r\n"));
#endif
 uint8_t sb=1;size_t sz=1;if(s->rt->get_variable(S("SecureBoot"),&global_var,0,&sz,&sb)||sz!=1||sb!=0)return SECURITY_VIOLATION;
 #ifdef FARKUAD_LIVE
 unsigned option=live_menu(s);if(option==3)return SUCCESS;
#endif
 uint64_t cr4;__asm__ volatile("mov %%cr4,%0":"=r"(cr4));if((cr4&(1ull<<12))||(uintptr_t)lab_jump>=0xfffff000ULL)return UNSUPPORTED;
 size_t n=(size_t)(kernel_payload_end-kernel_payload);if(!n||n>0x2e0000)return INVALID_PARAMETER;
 uint64_t base=0x100000;status r=s->bs->alloc_pages(2,2,768,&base);if(r)return r;if(base!=0x100000){s->bs->free_pages(base,768);return UNSUPPORTED;}
 zero((void*)(uintptr_t)base,0x300000);volatile unsigned char *dst=(void*)(uintptr_t)base;for(size_t i=0;i<n;i++)dst[i]=kernel_payload[i];
 struct swamp_bootinfo *b=(void*)SWAMP_UEFI_INFO;
#ifdef FARKUAD_LIVE
 b->reserved=1;b->live_option=option;
 size_t volume=(size_t)(live_volume_end-live_volume_start);
 uint64_t disk=PHYS_LIMIT-1;
 if(volume!=8*1024*1024){s->bs->free_pages(base,768);return UNSUPPORTED;}
 r=s->bs->alloc_pages(1,2,volume/4096,&disk);
 if(r){s->bs->free_pages(base,768);return r;}
 if(disk<SWAMP_UEFI_RESERVE||disk>PHYS_LIMIT-volume){s->bs->free_pages(disk,volume/4096);s->bs->free_pages(base,768);return UNSUPPORTED;}
 volatile unsigned char*ram=(void*)(uintptr_t)disk;for(size_t i=0;i<volume;i++)ram[i]=live_volume_start[i];
 b->ramdisk=disk;b->ramdisk_size=volume;
#endif
 if(s->entries<=1024&&s->config){struct config*c=s->config;for(size_t i=0;i<s->entries;i++){if(same(&c[i].id,&acpi2)){b->rsdp=(uintptr_t)c[i].table;break;}if(same(&c[i].id,&acpi1))b->rsdp=(uintptr_t)c[i].table;}}
 struct gop*g=0;if(!s->bs->locate_protocol(&gop_guid,0,(void**)&g)&&g&&g->mode&&g->mode->info&&g->mode->size>=sizeof(struct gop_info)){b->fb=g->mode->fb;b->width=g->mode->info->width;b->height=g->mode->info->height;b->stride=g->mode->info->stride;b->pixel_format=g->mode->info->format;b->fb_size=g->mode->fbsize;}
#ifdef FARKUAD_LIVE
 /* Firmware-reported BAR resources are metadata only; no PCI attributes are changed. */
 efi_collect_xhci_resources(s,b);
#endif
 uint64_t *pml4=(void*)0x3e1000,*pdpt=(void*)0x3e2000;pml4[0]=0x3e2003;
 for(unsigned i=0;i<4;i++){uint64_t a=0x3e3000+i*4096;pdpt[i]=a|3;uint64_t*pd=(void*)(uintptr_t)a;for(unsigned j=0;j<512;j++)pd[j]=((uint64_t)i*512+j)*0x200000|0x83;}
 uint64_t *gdt=(void*)0x3e7000;gdt[1]=0x00cf9a000000ffffULL;gdt[2]=0x00cf92000000ffffULL;gdt[3]=0x00af9a000000ffffULL;
 struct __attribute__((packed)){uint16_t limit;uint64_t address;} *gd=(void*)0x3e7020;gd->limit=31;gd->address=0x3e7000;
 say(s,S("LAB: requesting ExitBootServices\r\n"));
 for(unsigned retry=0;retry<3;retry++){size_t bytes=65536,key=0,stride=0;uint32_t version=0;void*map=(void*)0x3e8000;
  r=s->bs->get_map(&bytes,map,&key,&stride,&version);if(r||version!=1||!handoff_map(b,map,bytes,stride)){r=UNSUPPORTED;break;}
  r=s->bs->exit_boot(self,key);if(!r){transition();__builtin_unreachable();}if(r!=INVALID_PARAMETER)break;
 }
 /* After a failed ExitBootServices only memory allocation services are safe. */
#ifdef FARKUAD_LIVE
 s->bs->free_pages(disk,volume/4096);
#endif
 s->bs->free_pages(base,768);return r;
}
