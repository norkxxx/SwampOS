#include <stdint.h>
#include "core.h"
#include "task.h"
#include "vm.h"
#include "user.h"
#include "storage.h"
#include "pci.h"
#include "bootinfo.h"
#include "hardware.h"
#include "net.h"
#include "transport.h"
#include "swampwall.h"
#include "vpn_policy.h"
#include "virtio_net.h"
#include "device.h"
#include "audit.h"
#include "session.h"
#include "process.h"
#include "scratch.h"
#include "live.h"
#include "ps2.h"
#include "usb_input.h"
#include "lineedit.h"
#include "../drivers/usb/hid/boot_hid.h"
#include "../drivers/usb/xhci/ring.h"
#include "../drivers/usb/xhci/control.h"
#include "../drivers/usb/xhci/interrupt.h"
#include "../drivers/usb/core/enumeration.h"
#include "../drivers/rtc/rtc.h"
#include "../filesystem/ramfs/ramfs.h"
unsigned boot_uefi;
static volatile uint16_t *const vga=(void*)0xb8000;
static unsigned pos,serial_present;
static inline void out(uint16_t p,uint8_t v){__asm__ volatile("outb %0,%1"::"a"(v),"Nd"(p));}
static inline uint8_t in(uint16_t p){uint8_t v;__asm__ volatile("inb %1,%0":"=a"(v):"Nd"(p));return v;}
static void serial_init(void){out(0x3f9,0);out(0x3fb,0x80);out(0x3f8,3);out(0x3f9,0);out(0x3fb,3);out(0x3fa,0xc7);out(0x3fc,0x0b);out(0x3ff,0x5a);serial_present=in(0x3ff)==0x5a;out(0x3ff,0);}
static void put(char c){if(serial_present)for(unsigned i=0;i<100000;i++)if(in(0x3fd)&32){out(0x3f8,c);break;}desktop_put(c);if(boot_uefi)return;if(c=='\n')pos=(pos/80+1)*80;else if(c=='\b'){if(pos){--pos;vga[pos]=0x0720;}}else if(c!='\r'){if(pos<2000)vga[pos++]=0x0a00|(uint8_t)c;}if(pos>=2000){for(unsigned i=0;i<1920;i++)vga[i]=vga[i+80];for(unsigned i=1920;i<2000;i++)vga[i]=0x0720;pos=1920;}}
static void say(const char*s){while(*s)put(*s++);}
static int eq(const char*a,const char*b){while(*a&&*a==*b){a++;b++;}return *a==*b;}
static void clear(void){desktop_clear();if(boot_uefi)return;for(unsigned i=0;i<2000;i++)vga[i]=0x0720;pos=0;}
static void number(uint64_t n){char b[21];unsigned i=0;do{b[i++]='0'+n%10;n/=10;}while(n);while(i)put(b[--i]);}
void console_write(const char*s,size_t n){while(n--)put(*s++);}
void console_number(uint64_t n){number(n);}
static void hex(unsigned v,unsigned digits){const char *h="0123456789abcdef";while(digits){digits--;put(h[(v>>(digits*4))&15]);}}
static int pci_print(void*ctx,const struct pci_function*p){(void)ctx;hex(p->bus,2);put(':');hex(p->device,2);put('.');hex(p->function,1);put(' ');hex(p->vendor,4);put(':');hex(p->product,4);say(" class=");hex(p->class_code,2);hex(p->subclass,2);hex(p->prog_if,2);put('\n');return 1;}
static int device_print(void*ctx,const struct pci_function*p){(void)ctx;struct device_info d;if(!device_probe(pci_config_read,0,p,&d)||d.kind==DEV_OTHER)return 1;hex(p->bus,2);put(':');hex(p->device,2);put('.');hex(p->function,1);put(' ');say(device_name(d.kind));say(" caps=");number(d.cap_count);say(" PCIe=");number(d.pcie);say(" MSI=");number(d.msi);say(" MSI-X=");number(d.msix);say(" [inventory only]\n");return 1;}
static int usb_print(void*ctx,const struct pci_function*p){if(p->class_code!=12||p->subclass!=3)return 1;(*(unsigned*)ctx)++;pci_print(0,p);device_print(0,p);return 1;}
static void usb_inventory(void){unsigned n=0;pci_scan(pci_config_read,0,usb_print,&n);say("USB controller functions: ");number(n);say("\nInventory only; no USB transfers or HID activation.\n");}
static void two(unsigned n){put('0'+n/10);put('0'+n%10);}
static void rtc_print(void){struct rtc_time t;if(!rtc_hardware_time(&t)){say("RTC unavailable or unstable.\n");return;}say("RTC (century/timezone unknown): ");two(t.year2);put('-');two(t.month);put('-');two(t.day);put(' ');two(t.hour);put(':');two(t.minute);put(':');two(t.second);put('\n');}
static void audit_print(void){struct audit_event e;for(unsigned i=0;audit_get(&kernel_audit,i,&e);i++){say("seq=");number(e.seq);say(" kind=");number(e.kind);say(" result=");if(e.result<0){put('-');number((uint64_t)-(int64_t)e.result);}else number(e.result);say(" detail=");number(e.detail);put('\n');}}
static void command(char*s){if(eq(s,"help"))say("help clear version onion shrek status mem ticks selftest tasks vmtest sandbox pmm pci devices usb usbtest xhci-ring-test usb-enum-test xhci-control-test xhci-interrupt-test xhci xhci-fw xhci-probe iommu usb-start usb-stop usb-status rtc cpu firmware acpi net nettest netlab transport wall vpn-policy audit audit-test guard-list guard-set guard-show guard-reset persist-mount persist-umount ls /persist cat /persist/note write /persist/note TEXT ata-info gpt mount-gpt mount-live umount storage fscheck ls cat run run-console run-data run-resource run-resource-console session-test process-test scratch-test run-pair run-group run-scratch ramls ramstat ramappend ramimport ramwrite ramcat ramcp rammv ramrm rammkdir ramrmdir ramdirs gui-demo gui-async-test halt\n");else if(eq(s,"clear"))clear();else if(eq(s,"version"))say("SwampOS 0.20.0-dev.38-r1 Awakens / x86_64\n");else if(eq(s,"onion"))say("Security has layers. Experimental ring 3 isolation.\n");else if(eq(s,"shrek"))say("WHAT ARE YOU DOING IN MY SWAMP?!\n");else if(eq(s,"status"))say("Long mode; 4 KiB paging; NX; experimental ring 3. Native ELF SDK; host package tools.\n");else if(eq(s,"mem")){say("Arena pages used: ");number(pages_used());say(" / 32\n");}else if(eq(s,"ticks")){number(ticks());say("\n");}else if(eq(s,"selftest"))say(core_test()?"CORE_TEST_OK\n":"CORE_TEST_FAIL\n");else if(eq(s,"tasks")){task_demo();say("Task A: ");number(task_steps(0));say(" Task B: ");number(task_steps(1));say("\n");}else if(eq(s,"pmm")){say("Free frames: ");number(frames.free);say(" / ");number(frames.total);say("\n");}else if(eq(s,"vmtest"))say(vm_selftest()?"VM_TEST_OK\n":"VM_TEST_FAIL\n");else if(eq(s,"sandbox"))say(user_selftest()?"USER_TEST_OK\n":"USER_TEST_FAIL\n");else if(eq(s,"pci")){unsigned n=pci_scan(pci_config_read,0,pci_print,0);say("PCI functions: ");number(n);say("\n");}else if(eq(s,"devices")){pci_scan(pci_config_read,0,device_print,0);}else if(eq(s,"usb"))usb_inventory();else if(eq(s,"usbtest"))say(usb_hid_selftest()?"USB_HID_TEST_OK\n":"USB_HID_TEST_FAIL\n");else if(eq(s,"xhci-ring-test"))say(xhci_ring_selftest()?"XHCI_RING_TEST_OK (software only)\n":"XHCI_RING_TEST_FAIL\n");else if(eq(s,"usb-enum-test"))say(usb_enum_selftest()?"USB_ENUM_TEST_OK (software only)\n":"USB_ENUM_TEST_FAIL\n");else if(eq(s,"xhci-control-test"))say(xhci_control_selftest()?"XHCI_CONTROL_TEST_OK (software only)\n":"XHCI_CONTROL_TEST_FAIL\n");else if(eq(s,"xhci-interrupt-test"))say(xhci_interrupt_selftest()?"XHCI_INTERRUPT_TEST_OK (software only)\n":"XHCI_INTERRUPT_TEST_FAIL\n");else if(eq(s,"rtc"))rtc_print();else if(eq(s,"halt")){say("Swamp halted.\n");for(;;)__asm__ volatile("cli; hlt");}else if(eq(s,"transport"))say(transport_selftest()?"TRANSPORT_TEST_OK\n":"TRANSPORT_TEST_FAIL\n");else if(eq(s,"process-test"))say(proc_selftest()?"PROCESS_TEST_OK\n":"PROCESS_TEST_FAIL\n");else if(eq(s,"scratch-test"))say(scratch_selftest()?"SCRATCH_TEST_OK\n":"SCRATCH_TEST_FAIL\n");else if(eq(s,"session-test"))say(session_selftest()?"SESSION_TEST_OK\n":"SESSION_TEST_FAIL\n");else if(eq(s,"audit"))audit_print();else if(eq(s,"audit-test"))say(audit_selftest()?"AUDIT_TEST_OK\n":"AUDIT_TEST_FAIL\n");else if(eq(s,"wall"))say(wall_selftest()?"WALL_TEST_OK\n":"WALL_TEST_FAIL\n");else if(eq(s,"vpn-policy"))say(vpn_selftest()?"VPN_POLICY_TEST_OK\n":"VPN_POLICY_TEST_FAIL\n");else if(eq(s,"usb-start"))usb_input_start();else if(eq(s,"usb-stop"))usb_input_stop();else if(eq(s,"usb-status"))usb_input_status();else if(*s&&!ramfs_command(s)&&!hardware_command(s)&&!net_command(s)&&(boot_uefi&&((const struct swamp_bootinfo*)SWAMP_UEFI_INFO)->reserved==1?(!storage_command(s)):(!netlab_command(s)&&!storage_command(s))))say("Unknown command. Type help.\n");}
void kmain(void){serial_init();ramfs_init(&live_ramfs);clear();user_init();core_init();if(!vm_init()){say("VM_INIT_FAIL\n");for(;;)__asm__ volatile("cli; hlt");}desktop_start();unsigned input=ps2_init(desktop_active()&&((const struct swamp_bootinfo*)SWAMP_UEFI_INFO)->live_option==0);say(input&1?"PS2_KEYBOARD_ACK\n":"PS2_KEYBOARD_UNCONFIRMED\n");say(input&2?"PS2_MOUSE_ACK\n":"PS2_MOUSE_DISABLED_OR_UNAVAILABLE\n");if(desktop_active())say("LIVE_DESKTOP_READY\n");else if(boot_uefi&&((const struct swamp_bootinfo*)SWAMP_UEFI_INFO)->reserved==1){say(((const struct swamp_bootinfo*)SWAMP_UEFI_INFO)->live_option==2?"LIVE_SERIAL_RECOVERY\n":"LIVE_DESKTOP_UNAVAILABLE\n");}if(!vm_selftest()){say("VM_TEST_FAIL\n");goto fatal;}say("VM_TEST_OK\n");if(!user_selftest()){say("USER_TEST_FAIL\n");goto fatal;}say("USER_TEST_OK\n");if(!core_test()){say("CORE_TEST_FAIL\n");goto fatal;}say("CORE_TEST_OK\n");if(!net_selftest()){say("NET_TEST_FAIL\n");goto fatal;}say("NET_TEST_OK\n");if(!transport_selftest()||!wall_selftest()||!vpn_selftest()){say("CONNECTIVITY_TEST_FAIL\n");goto fatal;}say("CONNECTIVITY_TEST_OK\n");if(!audit_selftest()){say("AUDIT_TEST_FAIL\n");goto fatal;}say("AUDIT_TEST_OK\n");if(!session_selftest()){say("SESSION_TEST_FAIL\n");goto fatal;}say("SESSION_TEST_OK\n");if(!proc_selftest()||!scratch_selftest()){say("OVERLORD_TEST_FAIL\n");goto fatal;}say("OVERLORD_TEST_OK\n");if(storage_init()){if(!storage_selftest()){say("STORAGE_TEST_FAIL\n");goto fatal;}say("STORAGE_TEST_OK\n");}else say("STORAGE_NOT_MOUNTED\n");say("SwampOS 0.20.0-dev.38-r1 Awakens\nSecurity has layers.\nBOOT_OK\n");usb_input_auto();struct lineedit input_line;line_init(&input_line);say("swamp> ");for(;;){desktop_refresh();int key;struct mouse_event event;if(ps2_poll(&event,&key))desktop_mouse(event.dx,event.dy,event.buttons);if(!key&&usb_input_poll(&key,&event))desktop_mouse(event.dx,event.dy,event.buttons);if(!key&&serial_present){uint8_t lsr=in(0x3fd);if(lsr!=255&&(lsr&1))key=in(0x3f8);}if(!key)continue;if(desktop_key(key))continue;enum line_event e=line_key(&input_line,key);if(e==LINE_SUBMIT){put('\n');command(input_line.line);line_finish(&input_line);say("swamp> ");}else if(e==LINE_ERASE)put('\b');else if(e==LINE_ECHO)put((char)key);else if(e==LINE_REPAINT){say("\nswamp> ");say(input_line.line);}}fatal: desktop_refresh();for(;;)__asm__ volatile("cli; hlt");}
