#include "../drivers/usb/hid/boot_hid.h"
#include "../drivers/rtc/rtc.h"
#include "../kernel/device.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static unsigned seed=0x5a17;static unsigned rnd(void){seed=seed*1664525u+1013904223u;return seed;}
static const uint8_t cfg[]={9,2,41,0,2,1,0,0x80,50,9,4,0,0,1,3,1,1,0,7,5,0x81,3,8,0,10,9,4,1,0,1,3,1,2,0,7,5,0x82,3,3,0,5};
static uint8_t regs[16];static unsigned unstable,calls;
static uint8_t readreg(void*c,uint8_t r){(void)c;assert(r<sizeof regs);calls++;if(unstable&&r==0)return (uint8_t)(calls&7);return regs[r];}
static uint32_t pciread(void*c,uint8_t b,uint8_t d,uint8_t f,uint8_t off){(void)c;(void)b;(void)d;(void)f;return off==0?0x12348086:0;}
static void bcd(void){memset(regs,0,sizeof regs);regs[0]=0x59;regs[2]=0x58;regs[4]=0x23;regs[7]=0x29;regs[8]=2;regs[9]=0x24;regs[11]=2;regs[13]=128;unstable=calls=0;}
int main(void){
 assert(usb_hid_selftest());struct usb_boot_config c;assert(usb_boot_config_parse(cfg,sizeof cfg,&c)&&c.count==2&&c.interfaces[0].protocol==1&&c.interfaces[1].protocol==2);uint8_t d[sizeof cfg];
 for(size_t n=0;n<sizeof cfg;n++)assert(!usb_boot_config_parse(cfg,n,&c)&&!c.count);
 memcpy(d,cfg,sizeof d);d[9]=0;assert(!usb_boot_config_parse(d,sizeof d,&c));memcpy(d,cfg,sizeof d);d[22]=2;assert(!usb_boot_config_parse(d,sizeof d,&c));memcpy(d,cfg,sizeof d);d[27]=0;assert(!usb_boot_config_parse(d,sizeof d,&c));
 struct hid_keyboard k={0};struct hid_keys e;uint8_t r[8]={0,0,4,5};assert(hid_keyboard_report(&k,r,8,&e)&&e.count==2&&e.keys[0]=='a'&&e.keys[1]=='b');assert(hid_keyboard_report(&k,r,8,&e)&&!e.count);r[2]=1;assert(!hid_keyboard_report(&k,r,8,&e)&&!e.count);r[2]=4;assert(hid_keyboard_report(&k,r,8,&e)&&!e.count);memset(r,0,8);assert(hid_keyboard_report(&k,r,8,&e));r[2]=57;assert(hid_keyboard_report(&k,r,8,&e)&&k.caps);r[3]=4;assert(hid_keyboard_report(&k,r,8,&e)&&e.count==1&&e.keys[0]=='A');r[3]=5;r[0]=2;assert(hid_keyboard_report(&k,r,8,&e)&&e.count==1&&e.keys[0]=='b');r[3]=6;r[0]=1;assert(hid_keyboard_report(&k,r,8,&e)&&!e.count);
 struct hid_mouse m;uint8_t mr[3]={7,128,127};assert(hid_mouse_report(mr,3,&m)&&m.dx==-128&&m.dy==127&&m.buttons==7);assert(!hid_mouse_report(mr,2,&m));mr[0]=8;assert(!hid_mouse_report(mr,3,&m));
 for(unsigned i=0;i<50000;i++){memcpy(d,cfg,sizeof d);unsigned pos=rnd()%sizeof d;d[pos]^=(uint8_t)(rnd()>>16);if(usb_boot_config_parse(d,sizeof d,&c)){assert(c.count<=USB_BOOT_HID_MAX);for(unsigned j=0;j<c.count;j++)assert(c.interfaces[j].endpoint&128);}else assert(!c.count);for(unsigned j=0;j<8;j++)r[j]=(uint8_t)(rnd()>>24);if(hid_keyboard_report(&k,r,8,&e))assert(e.count<=6);else assert(!e.count);for(unsigned j=0;j<3;j++)mr[j]=(uint8_t)(rnd()>>24);if(hid_mouse_report(mr,3,&m))assert(m.dx>=-128&&m.dx<=127&&m.dy>=-128&&m.dy<=127&&m.buttons<=7);}
 puts("PASS: USB config bounds, composite boot HID endpoints, keyboard state/rollover/modifiers, mouse signed motion; 50000 mutations and 100000 report samples");
 struct rtc_time t;bcd();assert(rtc_read_time(readreg,0,&t)&&t.year2==24&&t.month==2&&t.day==29&&t.hour==23&&t.minute==58&&t.second==59);regs[11]=0;regs[4]=0x92;assert(rtc_read_time(readreg,0,&t)&&t.hour==12);regs[4]=0x12;assert(rtc_read_time(readreg,0,&t)&&!t.hour);regs[4]=0x81;assert(rtc_read_time(readreg,0,&t)&&t.hour==13);regs[11]=6;regs[4]=23;regs[0]=59;regs[2]=58;regs[7]=29;regs[8]=2;regs[9]=24;assert(rtc_read_time(readreg,0,&t)&&t.hour==23);regs[9]=23;assert(!rtc_read_time(readreg,0,&t));bcd();regs[0]=0x6a;assert(!rtc_read_time(readreg,0,&t));bcd();regs[10]=128;assert(!rtc_read_time(readreg,0,&t)&&calls<5000);bcd();unstable=1;assert(!rtc_read_time(readreg,0,&t));bcd();regs[13]=0;assert(!rtc_read_time(readreg,0,&t));
 for(unsigned i=0;i<50000;i++){bcd();regs[rnd()%14]^=(uint8_t)(rnd()>>16);if(rtc_read_time(readreg,0,&t))assert(t.month>=1&&t.month<=12&&t.day>=1&&t.day<=31&&t.hour<=23&&t.minute<=59&&t.second<=59&&t.year2<=99);}
 puts("PASS: RTC binary/BCD, 12/24 hour conversion, date bounds, stuck UIP and unstable sampling; 50000 register mutations");
 struct pci_function p={.vendor=0x8086,.product=0x1234,.class_code=12,.subclass=3};struct device_info info;enum device_kind kinds[]={DEV_UHCI,DEV_OHCI,DEV_EHCI,DEV_XHCI};for(unsigned i=0;i<4;i++){p.prog_if=i*16;assert(device_probe(pciread,0,&p,&info)&&info.kind==kinds[i]);}p.prog_if=0x80;assert(device_probe(pciread,0,&p,&info)&&info.kind==DEV_OTHER);puts("PASS: UHCI/OHCI/EHCI/xHCI inventory");
}
