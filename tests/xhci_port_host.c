#include "../drivers/usb/xhci/port.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#define OP 0x40
#define RT 0x600
#define DB 0x800
#define EXT 0x900
#define PORT (OP+0x400)
#define CCS 1u
#define PED 2u
#define PR 16u
#define PP 512u
#define PRC (1u<<21)
#define CSC (1u<<17)
struct mock {uint32_t r[1024];unsigned elapsed,reset,mode,writes;};
static int read32(void*p,uint32_t o,uint32_t*v){struct mock*m=p;if(o>=sizeof m->r)return 0;*v=m->r[o/4];return 1;}
static int write32(void*p,uint32_t o,uint32_t v){struct mock*m=p;if(o>=sizeof m->r||o!=PORT)return 0;assert(!(v&(PED|CSC|(1u<<31)|(1u<<16))));m->writes++;uint32_t old=m->r[o/4];if(v&PRC)old&=~PRC;if(v&PP)old|=PP;else old&=~PP;if(v&PR){old|=PR;old&=~PED;m->reset=m->elapsed;}m->r[o/4]=old;return 1;}
static void delay(void*p){struct mock*m=p;m->elapsed++;if(m->mode==1&&m->elapsed==50)m->r[PORT/4]&=~CCS;if(m->reset&&m->elapsed-m->reset==20&&m->mode!=2){m->r[PORT/4]&=~PR;m->r[PORT/4]|=PED|PRC|(m->mode==3?4u:3u)<<10;}}
static void init(struct mock*m,struct xhci_controller*c){memset(m,0,sizeof *m);m->r[0]=0x01100040;m->r[1]=0x02000108;m->r[4]=1|8|((EXT/4)<<16);m->r[5]=DB;m->r[6]=RT;m->r[EXT/4]=(2u<<24)|(4u<<8)|2;m->r[EXT/4+1]=0x20425355;m->r[EXT/4+2]=0x0101;m->r[EXT/4+4]=(3u<<24)|2;m->r[EXT/4+5]=0x20425355;m->r[EXT/4+6]=0x0102;m->r[PORT/4]=CCS|PP|PED|CSC|PRC;*c=(struct xhci_controller){.io={m,read32,write32,delay,sizeof m->r},.state=2};assert(xhci_caps_read(&c->io,&c->caps));}
int main(void){struct mock m;struct xhci_controller c;for(unsigned i=0;i<10000;i++){init(&m,&c);assert(xhci_usb2_port(&c,1)&&!xhci_usb2_port(&c,2));if(i&1)m.r[PORT/4]&=~PP;assert(xhci_port_reset(&c,1)==3&&!(m.r[PORT/4]&PRC)&&(m.r[PORT/4]&CSC));assert(m.elapsed==(i&1?140:120)&&m.writes==(i&1?4:3));}for(unsigned mode=1;mode<=3;mode++){init(&m,&c);m.mode=mode;assert(!xhci_port_reset(&c,1));assert(m.elapsed<1201);}init(&m,&c);assert(!xhci_port_reset(&c,2)&&!m.writes);m.r[EXT/4+2]|=1u<<28;assert(!xhci_port_reset(&c,1)&&!m.writes);init(&m,&c);m.r[EXT/4+6]=0x0101;assert(!xhci_usb2_port(&c,1));init(&m,&c);m.r[PORT/4]|=8;assert(!xhci_port_reset(&c,1)&&!m.writes);init(&m,&c);m.r[EXT/4]=(2u<<24)|(1u<<8)|2;assert(!xhci_usb2_port(&c,1));puts("XHCI_PORT_HOST_OK: 10000 USB2 reset cycles, power/debounce, W1C preservation, USB3/PSI rejection, disconnect/timeout/overcurrent");}
