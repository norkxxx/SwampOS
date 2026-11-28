#include "hardware.h"
#include "bootinfo.h"
extern unsigned boot_uefi;
#include "pci.h"
#include "../drivers/usb/xhci/pci_inventory.h"
#include "vm.h"
#include "../drivers/usb/xhci/controller.h"
static void cpuid(uint32_t a,uint32_t c,uint32_t*r){__asm__ volatile("cpuid":"=a"(r[0]),"=b"(r[1]),"=c"(r[2]),"=d"(r[3]):"a"(a),"c"(c));}
void cpu_probe(struct cpu_info*i){*i=(struct cpu_info){0};uint32_t r[4];cpuid(0,0,r);uint32_t max=r[0];for(unsigned j=0;j<4;j++){i->vendor[j]=r[1]>>(8*j);i->vendor[4+j]=r[3]>>(8*j);i->vendor[8+j]=r[2]>>(8*j);}if(max>=1){cpuid(1,0,r);i->leaf1_ecx=r[2];i->leaf1_edx=r[3];i->logical=(r[1]>>16)&255;}if(max>=7){cpuid(7,0,r);i->leaf7_ebx=r[1];}cpuid(0x80000000,0,r);if(r[0]>=0x80000001){cpuid(0x80000001,0,r);i->ext_edx=r[3];}}
extern void console_write(const char *,size_t),console_number(uint64_t);
static void say(const char*s){size_t n=0;while(s[n])n++;console_write(s,n);}
static int eq(const char*a,const char*b){while(*a&&*a==*b){a++;b++;}return *a==*b;}
static void field(const char*s,uint64_t n){say(s);console_number(n);say("\n");}
static void hexn(uint64_t n,unsigned digits){static const char h[]="0123456789abcdef";while(digits){digits--;console_write(&h[(n>>(digits*4))&15],1);}}
struct xhci_inventory_state { unsigned count, rejected; };
static int xhci_inventory_visit(void *ctx,const struct pci_function *p){
 struct xhci_inventory_state *state=ctx;
 if(p->class_code!=0x0c||p->subclass!=0x03||p->prog_if!=0x30)return 1;
 struct xhci_pci_info x;
 if(!xhci_pci_inspect(pci_config_read,0,p,&x)){state->rejected++;return 1;}
 state->count++;
 say("xHCI ");hexn(x.bus,2);say(":");hexn(x.device,2);say(".");hexn(x.function,1);
 say(" id=");hexn(x.vendor,4);say(":");hexn(x.product,4);
 say(" BAR0=");if(!x.bar_assigned)say("unassigned");else {say("0x");hexn(x.bar0, x.bar_kind==3?16:8);say(x.bar_kind==3?" mem64":" mem32");}
 say(" MEM=");say(x.memory_enabled?"on":"off");
 say(" MASTER=");say(x.bus_master_enabled?"on":"off");
 say(" IO=");say(x.io_enabled?"on":"off");say(" [read-only]\n");
 return 1;
}
static void xhci_inventory(void){
 struct xhci_inventory_state state={0};
 unsigned scanned=pci_scan(pci_config_read,0,xhci_inventory_visit,&state);
 if(!state.count)say("No valid xHCI PCI functions found.\n");
 field("PCI functions scanned: ",scanned);
 field("xHCI functions: ",state.count);
 field("Rejected xHCI records: ",state.rejected);
 say("Read-only PCI inspection. BAR sizes are not probed; no config writes, MMIO access or DMA activation.\n");
}
static int xhci_fw_record_matches(const struct swamp_xhci_resource *r){
 return xhci_pci_resource_matches(pci_config_read,0,r);
}
static void xhci_firmware_inventory(void){
 if(!boot_uefi){say("UEFI PCI I/O handoff unavailable in BIOS mode.\n");return;}
 const struct swamp_bootinfo *b=(const void*)SWAMP_UEFI_INFO;
 if(b->xhci_magic!=SWAMP_XHCI_HANDOFF_MAGIC||b->xhci_version!=SWAMP_XHCI_HANDOFF_VERSION||b->xhci_count>SWAMP_XHCI_RESOURCE_MAX){say("No valid firmware PCI resource handoff.\n");return;}
 if(!b->xhci_count)say("UEFI reported no xHCI BAR0 resources.\n");
 for(unsigned i=0;i<b->xhci_count;i++){
  const struct swamp_xhci_resource *r=&b->xhci[i];
  if(r->segment!=0){say("Skipped nonzero PCI segment; ECAM backend not present.\n");continue;}
  if(!xhci_fw_record_matches(r)){say("Rejected firmware resource: current PCI configuration does not match.\n");continue;}
  say("UEFI xHCI ");hexn(r->segment,4);say(":");hexn(r->bus,2);say(":");hexn(r->device,2);say(".");hexn(r->function,1);
  say(" id=");hexn(r->vendor,4);say(":");hexn(r->product,4);
  say(" BAR0=0x");hexn(r->base,16);say(" length=0x");hexn(r->length,8);
  say(" MEM=");say((r->flags&1)?"on":"off");say(" MASTER=");say((r->flags&2)?"on":"off");
  say(" IO=");say((r->flags&4)?"on":"off");say((r->flags&8)?" mem64":" mem32");say(" [PCI revalidated]\n");
 }
 say("Metadata only. No BAR writes, PCI attribute changes, MMIO access or DMA activation.\n");
}
static int xhci_mmio_read32(void *ctx,uint32_t offset,uint32_t *value){
 if(!ctx||!value||(offset&3u))return 0;
 volatile const uint32_t *reg=(volatile const uint32_t*)((uintptr_t)ctx+offset);
 *value=*reg;return 1;
}
static void xhci_readonly_probe(void){
 if(!boot_uefi){say("xHCI MMIO probe requires UEFI resource handoff.\n");return;}
 const struct swamp_bootinfo *b=(const void*)SWAMP_UEFI_INFO;
 if(b->xhci_magic!=SWAMP_XHCI_HANDOFF_MAGIC||b->xhci_version!=SWAMP_XHCI_HANDOFF_VERSION||b->xhci_count>SWAMP_XHCI_RESOURCE_MAX||!b->phys_complete){say("No complete, versioned UEFI PCI/MMIO resource handoff.\n");return;}
 for(unsigned n=0;n<b->xhci_count;n++){
  const struct swamp_xhci_resource *r=&b->xhci[n];
  if(r->segment!=0||!xhci_fw_record_matches(r))continue;
  if(!(r->flags&1u)){say("xHCI BAR0 Memory Space is disabled; refusing MMIO read.\n");return;}
  if(!vm_mmio_matches(r->base,r->length)&&!vm_map_mmio(r->base,r->length)){say("UC MMIO mapping rejected: PAT, E820 RAM alias, framebuffer overlap, aperture or memory limit.\n");return;}
  struct xhci_io io={.ctx=(void*)(uintptr_t)VM_MMIO_VIRT,.read32=xhci_mmio_read32,.write32=0,.delay_ms=0,.aperture=(uint32_t)r->length};
  struct xhci_caps caps;uint32_t version;
  if(!xhci_caps_read(&io,&caps)||!xhci_mmio_read32(io.ctx,0,&version)){say("xHCI capability register layout rejected; no writes performed.\n");return;}
  say("xHCI CAPABILITIES [read-only]\nHCI version: 0x");hexn(version>>16,4);
  say(" CAPLENGTH: 0x");hexn(caps.op,2);
  say(" MaxSlots: ");console_number(caps.slots);say(" MaxPorts: ");console_number(caps.ports);
  say(" Scratchpads: ");console_number(caps.scratchpads);say(" AC64: ");say(caps.ac64?"yes":"no");
  say(" Context: ");console_number(caps.context_stride);say(" bytes PPC: ");say(caps.ppc?"yes":"no");
  say("Operational: 0x");hexn(caps.op,4);say(" Runtime: 0x");hexn(caps.rt,4);say(" Doorbell: 0x");hexn(caps.db,4);
  say(" Extended Capabilities: 0x");hexn(caps.ext,4);say("\nNo controller reset/start, PCI writes, Bus Master changes or DMA.\n");return;
 }
 say("No revalidated segment-0 xHCI firmware resource is available.\n");
}
static int phys(void*c,uint64_t a,void*out,size_t n){(void)c;if(a<4096||a>=PHYS_LIMIT||n>PHYS_LIMIT-a)return 0;const volatile uint8_t*p=(const void*)(uintptr_t)a;uint8_t*q=out;while(n--)*q++=*p++;return 1;}
static int scan(uint64_t start,uint64_t end,struct acpi_info *i){for(uint64_t a=start;a+20<=end;a+=16)if(acpi_probe(phys,0,a,i))return 1;return 0;}
int acpi_get_info(struct acpi_info *a){
 if(!a)return 0;
 if(boot_uefi){const struct swamp_bootinfo*b=(const void*)SWAMP_UEFI_INFO;return b->rsdp&&acpi_probe(phys,0,b->rsdp,a);}
 uint16_t segment=0;if(phys(0,0x4ff2,&segment,2)&&segment>=0x8000&&segment<=0x9fc0&&scan((uint64_t)segment<<4,((uint64_t)segment<<4)+1024,a))return 1;
 return scan(0xe0000,0x100000,a);
}
static void iommu_inventory(void){
 struct acpi_info a;if(!acpi_get_info(&a)){say("ACPI unavailable within mapped physical window.\n");return;}
 if(!a.dmar){say("Intel DMAR not present; AMD IVRS parsing is not implemented.\n");return;}
 if(a.dmar_bad||a.dmar_truncated){say("DMAR table malformed or inventory truncated; IOMMU coverage is not trusted.\n");return;}
 say("Intel VT-d DMAR present [inventory only]\n");field("Host address width: ",(unsigned)a.dmar_host_width+1);field("DRHD units: ",a.dmar_drhd_count);field("RMRR regions: ",a.dmar_rmrr_count);field("Device scopes: ",a.dmar_scope_count);field("Interrupt remapping advertised: ",a.dmar_flags&1u);
 unsigned units=a.dmar_drhd_count<ACPI_DMAR_MAX_DRHD?a.dmar_drhd_count:ACPI_DMAR_MAX_DRHD;
 for(unsigned i=0;i<units;i++){const struct acpi_dmar_unit*u=&a.drhd[i];say("DRHD ");console_number(i);say(" segment=");hexn(u->segment,4);say(" register=0x");hexn(u->reg_base,16);say(" include-all=");say((u->flags&1u)?"yes":"no");say("\n");}
 if(boot_uefi){const struct swamp_bootinfo*b=(const void*)SWAMP_UEFI_INFO;if(b->xhci_magic==SWAMP_XHCI_HANDOFF_MAGIC&&b->xhci_version==SWAMP_XHCI_HANDOFF_VERSION&&b->xhci_count<=SWAMP_XHCI_RESOURCE_MAX){for(unsigned i=0;i<b->xhci_count;i++){const struct swamp_xhci_resource*r=&b->xhci[i];if(!xhci_pci_resource_matches(pci_config_read,0,r)){say("xHCI resource not live-revalidated; coverage not reported.\n");continue;}say("xHCI ");hexn(r->segment,4);say(":");hexn(r->bus,2);say(":");hexn(r->device,2);say(".");hexn(r->function,1);say(" IOMMU coverage: ");say(acpi_dmar_covers(&a,r->segment,r->bus,r->device,r->function)?"DMAR scope matched":"not proven");say("\n");}}}
 say("DMAR is firmware metadata only. No VT-d registers, remapping tables, PCI attributes or DMA are changed.\n");
}
int hardware_command(const char*s){
 if(eq(s,"xhci")){xhci_inventory();return 1;}
 if(eq(s,"xhci-fw")){xhci_firmware_inventory();return 1;}
 if(eq(s,"xhci-probe")){xhci_readonly_probe();return 1;}
 if(eq(s,"cpu")){struct cpu_info c;cpu_probe(&c);say("CPU: ");say(c.vendor);say("\n");field("CPUID logical IDs: ",c.logical);field("APIC: ",(c.leaf1_edx>>9)&1);field("VMX: ",(c.leaf1_ecx>>5)&1);field("NX: ",(c.ext_edx>>20)&1);field("SMEP: ",(c.leaf7_ebx>>7)&1);field("SMAP: ",(c.leaf7_ebx>>20)&1);return 1;}
 if(eq(s,"firmware")){say(boot_uefi?"UEFI plaintext kernel lab\n":"BIOS plaintext lab\n");if(boot_uefi){const struct swamp_bootinfo*b=(const void*)SWAMP_UEFI_INFO;field("GOP width: ",b->width);field("GOP height: ",b->height);field("GOP stride: ",b->stride);field("GOP pixel format: ",b->pixel_format);}return 1;}
 if(eq(s,"acpi")){struct acpi_info a;if(!acpi_get_info(&a)){say("ACPI unavailable within mapped physical window.\n");return 1;}field("ACPI enabled processors: ",a.cpus);field("IOAPIC count: ",a.ioapics);field("IRQ overrides: ",a.overrides);field("MCFG present: ",!!a.mcfg);field("DMAR present: ",!!a.dmar);return 1;}
 if(eq(s,"iommu")){iommu_inventory();return 1;}
 return 0;
}
