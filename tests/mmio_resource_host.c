#include "vm.h"
#include "../drivers/usb/xhci/pci_resource.h"
#include <assert.h>
#include <stdio.h>
#define ADDR 0x000ffffffffff000ull
extern int vm_host_init(void);
extern uint64_t vm_host_entry(uint64_t,unsigned);
static uint32_t regs[64];
static uint32_t pci(void*p,uint8_t b,uint8_t d,uint8_t f,uint8_t off){(void)p;return b==0&&d==5&&f==0?regs[off/4]:UINT32_MAX;}
int main(void){regs[0]=0x12348086;regs[1]=2;regs[2]=0x0c033001;regs[4]=0xf0000004;regs[5]=0;struct xhci_pci_resource r;assert(xhci_pci_resource(pci,0,0,5,0,0xf0000000,65536,&r)&&r.memory_enabled&&!r.bus_master&&r.base==0xf0000000);assert(!xhci_pci_resource(pci,0,0,5,0,0xe0000000,65536,&r));regs[4]|=8;assert(!xhci_pci_resource(pci,0,0,5,0,0xf0000000,65536,&r));regs[4]=0xf0000004;regs[5]=1;assert(xhci_pci_resource(pci,0,0,5,0,0x1f0000000,65536,&r));regs[5]=0;regs[1]=6;assert(xhci_pci_resource(pci,0,0,5,0,0xf0000000,65536,&r)&&r.bus_master);regs[2]=0x0c032001;assert(!xhci_pci_resource(pci,0,0,5,0,0xf0000000,65536,&r));
 volatile uint32_t mock[1024]={0};struct xhci_mmio_window w={mock,sizeof mock,0};uint32_t val;assert(!xhci_mmio_write32(&w,0,1)&&xhci_mmio_read32(&w,0,&val)&&!val);w.writable=1;for(unsigned i=0;i<10000;i++){unsigned off=(i%1024)*4;assert(xhci_mmio_write32(&w,off,i)&&xhci_mmio_read32(&w,off,&val)&&val==i);assert(!xhci_mmio_read32(&w,off+1,&val)&&!xhci_mmio_write32(&w,sizeof mock,i));}
 assert(vm_host_init());unsigned before=frames.free;uint64_t root=vm_kernel_root();assert(!vm_map_mmio(0x100000,4096)&&!vm_map_mmio(0xf0000001,4096)&&!vm_map_mmio(0xf0000000,VM_MMIO_LIMIT+4096));assert(vm_map_live_fb(0xe0000000,8192));assert(!vm_map_mmio(0xe0001000,4096));
 struct e820_entry high_ram={0xf0000000,0x100000,1,1};assert(vm_host_set_e820(&high_ram,1));assert(!vm_map_mmio(0xf0000000,65536));
 struct e820_entry high_reserved={0xf0000000,0x100000,2,1};assert(vm_host_set_e820(&high_reserved,1));
 static uint64_t taken[FRAME_COUNT];unsigned k=0;while(frames.free>2){taken[k]=pmm_take(&frames);assert(taken[k]);k++;}assert(!vm_map_mmio(0xf0000000,65536)&&frames.free==2&&!vm_host_entry(root,2));for(unsigned i=0;i<k;i++)assert(pmm_release(&frames,taken[i]));assert(frames.free<before);
 unsigned prev=frames.free;assert(vm_map_mmio(0xf0000000,65536)&&vm_mmio_matches(0xf0000000,65536)&&vm_map_mmio(0xf0000000,65536)&&frames.free==prev-3);uint64_t a=vm_host_entry(root,2),b=vm_host_entry(a&ADDR,0),c=vm_host_entry(b&ADDR,0);assert(!(a&4)&&!(b&4)&&!(c&4));for(unsigned i=0;i<16;i++)assert(vm_host_entry(c&ADDR,i)==((0xf0000000ull+4096*i)|0x800000000000001bull));assert(!vm_host_entry(c&ADDR,16));assert(!vm_map_mmio(0xf1000000,4096)&&!vm_mmio_matches(0xf1000000,4096));assert(!vm_map_iommu(0xf0001000)&&!vm_map_iommu(0xe0001000)&&!vm_map_iommu(0x100000));
 assert(vm_map_iommu(0xfed90000)&&vm_map_iommu(0xfed90000)&&!vm_map_iommu(0xfed91000));
 uint64_t ip=vm_host_entry(root,3),id=vm_host_entry(ip&ADDR,0),it=vm_host_entry(id&ADDR,0);
 assert(!(ip&4)&&!(id&4)&&!(it&4)&&vm_host_entry(it&ADDR,0)==(0xfed90000ull|0x800000000000001bull));
 assert(!vm_host_entry(it&ADDR,1));
 uint8_t code=0xc3;struct space s;assert(vm_create(&s,&code,1)&&!vm_host_entry(s.root,2)&&!vm_host_entry(s.root,3));vm_destroy(&s);
 puts("MMIO_RESOURCE_HOST_OK: PCI BAR validation, 10000 register transactions, UC/NX/supervisor flags, E820 RAM-alias rejection, idempotent map and OOM rollback");}
