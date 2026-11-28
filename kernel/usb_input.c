#include "fwcfg.h"
#include "usb_input.h"
#include "bootinfo.h"
#include "core.h"
#include "hardware.h"
#include "vm.h"
#include "vtd.h"
#include "../drivers/usb/xhci/pci_inventory.h"
#include "../drivers/usb/xhci/pci_resource.h"
#include "../drivers/usb/xhci/ownership.h"
#include "../drivers/usb/xhci/boot_session.h"

extern void console_write(const char *,size_t),console_number(uint64_t);
static void logline(const char *s){size_t n=0;while(s[n])n++;console_write(s,n);}
enum phase { IDLE, ACTIVE, QUARANTINED };
static enum phase phase;
static struct xhci_boot_session session;
static struct dma_lease memory[USB_DMA_WINDOWS];
static struct xhci_mmio_window window;
static struct swamp_xhci_resource device;
static int owned;
static struct hid_keys pending;
static unsigned next_key, reports;
static uint64_t last_poll;

static int enabled(void){return q35_live_option("opt/swamp/usb-input");}
static int master(int on){
 uint32_t addr=0x80000004u|((unsigned)device.bus<<16)|((unsigned)device.device<<11)|((unsigned)device.function<<8);
 uint16_t cmd=(device.command&~4u)|(on?4u:0u);
 uint64_t flags;
 __asm__ volatile("pushfq; pop %0; cli":"=r"(flags)::"memory");
 __asm__ volatile("outl %0,%1"::"a"(addr),"Nd"((uint16_t)0xcf8));
 __asm__ volatile("outw %0,%1"::"a"(cmd),"Nd"((uint16_t)0xcfc));
 if(flags&(1u<<9))__asm__ volatile("sti":::"memory");
 return (uint16_t)pci_config_read(0,device.bus,device.device,device.function,4)==cmd;
}
static void delay(void *ctx){
 (void)ctx;uint64_t start=ticks();
 while(ticks()==start)__asm__ volatile("hlt");
}
static unsigned connected_ports(struct xhci_io *io,unsigned ports[2]){
 struct xhci_controller c={.io=*io};
 if(!xhci_caps_read(io,&c.caps)||c.caps.scratchpads)return 0;
 unsigned selected=0;
 for(unsigned p=1;p<=c.caps.ports;p++){
  uint32_t status;
  if(!io->read32(io->ctx,c.caps.op+0x400+(p-1)*16,&status))return 0;
  if(!(status&1))continue;
  if(selected==2||!xhci_usb2_port(&c,p))return 0;
  ports[selected++]=p;
 }
 return selected;
}
static int release_memory(void){
 for(unsigned i=0;i<USB_DMA_WINDOWS;i++)if(memory[i].pages&&!vm_dma_release(&memory[i],1))return 0;
 return 1;
}
static void quarantine(void){
 phase=QUARANTINED;logline("USB_INPUT_QUARANTINED: reboot to reinitialize\n");
}
static void detach(void){
 int halted=!session.state||xhci_boot_stop(&session);
 int blocked=master(0);
 int revoked=vtd_revoke();
 pending.count=next_key=0;
 if(!halted||!blocked||!revoked||!release_memory()){quarantine();return;}
 session=(struct xhci_boot_session){0};phase=IDLE;
 logline("USB_INPUT_STOPPED: bus master off, domain revoked, buffers released\n");
}
void usb_input_stop(void){if(phase==ACTIVE)detach();else usb_input_status();}
void usb_input_status(void){
 logline(phase==ACTIVE?"USB input active, reports ":phase==QUARANTINED?"USB input quarantined, reports ":"USB input idle, reports ");
 console_number(reports);logline("\n");
}
void usb_input_start(void){
 if(phase!=IDLE){usb_input_status();return;}
 if(!enabled()){logline("USB_INPUT_DISABLED: Q35 Live opt-in required\n");return;}
 const struct swamp_bootinfo *b=(const void*)SWAMP_UEFI_INFO;
 if(!b->phys_complete||b->xhci_magic!=SWAMP_XHCI_HANDOFF_MAGIC||b->xhci_version!=SWAMP_XHCI_HANDOFF_VERSION||b->xhci_count!=1){
  logline("USB_INPUT_REFUSED: firmware resource inventory\n");return;
 }
 struct swamp_xhci_resource r=b->xhci[0];
 if(owned){r.command&=~4u;r.flags&=~2u;}
 if(r.vendor!=0x1b36||r.product!=0x000d||!(r.flags&1)||!xhci_pci_resource_matches(pci_config_read,0,&r)){
  logline("USB_INPUT_REFUSED: PCI identity\n");return;
 }
 struct acpi_info acpi;
 if(!acpi_get_info(&acpi)||acpi.dmar_drhd_count!=1||acpi.dmar_rmrr_count||
    !acpi_dmar_covers(&acpi,0,r.bus,r.device,r.function)||!vtd_available(acpi.drhd[0].reg_base)){
  logline("USB_INPUT_REFUSED: VT-d domain unavailable\n");return;
 }
 if(!vm_map_mmio(r.base,r.length)){logline("USB_INPUT_REFUSED: controller aperture\n");return;}
 window=(struct xhci_mmio_window){(void*)(uintptr_t)VM_MMIO_VIRT,(uint32_t)r.length,0};
 struct xhci_io io={&window,xhci_mmio_read32,xhci_mmio_write32,delay,window.bytes};
 unsigned ports[2],count=connected_ports(&io,ports);
 if(!count){logline("USB_INPUT_REFUSED: attach one or two USB2 HID devices\n");return;}
 const size_t size[]={sizeof(struct xhci_dma),sizeof(struct xhci_device_dma),sizeof(struct xhci_ep0_dma),sizeof(struct xhci_hid_dma),
                     sizeof(struct xhci_device_dma),sizeof(struct xhci_ep0_dma),sizeof(struct xhci_hid_dma)};
 unsigned windows=1+3*count;
 for(unsigned i=0;i<windows;i++)if(!vm_dma_alloc((size[i]+4095)/4096,1,&memory[i])){
  if(!release_memory())quarantine();
  logline("USB_INPUT_REFUSED: pinned memory\n");return;
 }
 window.writable=1;device=r;
 if(!xhci_legacy_claim(&io,xhci_mmio_write8)){if(!release_memory())quarantine();logline("USB_INPUT_REFUSED: firmware ownership\n");return;}
 if(!master(0)){quarantine();return;}
 owned=1;
 if(!vtd_bind(r.bus,(r.device<<3)|r.function,memory,windows)){quarantine();return;}
 /* Clear firmware queue state before permitting the controller to originate DMA. */
 struct xhci_caps caps;
 if(!xhci_caps_read(&io,&caps)||!io.write32(io.ctx,caps.op,0)){(void)vtd_revoke();quarantine();return;}
 uint32_t status=0;
 for(unsigned n=0;n<100 && !(status&1);n++){
  if(!io.read32(io.ctx,caps.op+4,&status))break;
  if(!(status&1))delay(0);
 }
 if(!(status&1)){(void)vtd_revoke();quarantine();return;}
 if(!master(1)){detach();return;}
 struct xhci_boot_buffers buffers[2];
 for(unsigned i=0;i<count;i++){
  unsigned j=1+3*i;
  buffers[i]=(struct xhci_boot_buffers){(void*)(uintptr_t)memory[0].base,(void*)(uintptr_t)memory[j].base,
   (void*)(uintptr_t)memory[j+1].base,(void*)(uintptr_t)memory[j+2].base,USB_IOVA(0),USB_IOVA(j),USB_IOVA(j+1),USB_IOVA(j+2)};
 }
 if(!xhci_boot_start_ports(&session,&io,buffers,ports,count)){logline("USB_INPUT_REFUSED: HID enumeration\n");detach();return;}
 phase=ACTIVE;
 for(unsigned i=0;i<count;i++){
  unsigned protocol=i?session.second_hid.protocol:session.hid.protocol;
  logline(protocol==1?"USB_INPUT_KEYBOARD_READY\n":"USB_INPUT_MOUSE_READY\n");
 }
}
void usb_input_auto(void){if(enabled())usb_input_start();}
int usb_input_poll(int *key,struct mouse_event *mouse){
 if(phase!=ACTIVE)return 0;
 if(next_key<pending.count){*key=pending.keys[next_key++];return 0;}
 uint64_t now=ticks();if(last_poll==now)return 0;last_poll=now;
 struct xhci_hid_event event;
 int got=xhci_boot_poll(&session,&event);
 if(got<0){detach();return 0;}
 if(!got)return 0;
 reports++;
 if(event.protocol==2){*mouse=(struct mouse_event){event.mouse.dx,event.mouse.dy,event.mouse.buttons};return 1;}
 pending=event.keys;next_key=0;if(pending.count)*key=pending.keys[next_key++];
 return 0;
}
