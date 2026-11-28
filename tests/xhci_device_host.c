#include "../drivers/usb/xhci/device.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#define OP 0x40
#define RT 0x600
#define DB 0x800
#define CDMA 0x100000
#define DDMA 0x200000
#define EDMA 0x300000
struct model {uint32_t regs[1024];struct xhci_dma*c;struct xhci_device_dma*d;unsigned head,cycle,ev,ec,slot,mode,commands;};
static int read32(void*p,uint32_t o,uint32_t*v){struct model*m=p;if(o>=sizeof m->regs)return 0;*v=m->regs[o/4];return 1;}
static void command(struct model*m){unsigned idx=m->head;struct xhci_trb t=m->c->cmd[idx];unsigned type=(t.control>>10)&63,slot=t.control>>24;assert((t.control&1)==m->cycle);m->commands++;if(type==9){assert(!slot&&!m->slot&&!t.lo&&!t.hi);m->slot=1;}else if(type==11){assert(slot==1&&m->slot==1&&t.lo==DDMA&&!t.hi);assert(m->c->dcbaa[1]==DDMA+offsetof(struct xhci_device_dma,output));unsigned stride=(m->regs[4]&4)?16:8;assert(m->d->input[1]==3&&m->d->input[stride]==((3u<<20)|(1u<<27))&&m->d->input[stride+1]==1u<<16&&m->d->input[stride*2+2]==(EDMA|1));m->d->output[3]=(2u<<27)|7;m->slot=2;}else if(type==10){assert(slot==1&&m->slot==2&&m->c->dcbaa[1]);m->slot=0;}else assert(0);if(m->mode==2&&type==11)m->d->output[3]=0;unsigned outslot=m->mode==1?2:1;m->c->event[m->ev]=(struct xhci_trb){CDMA+idx*16,0,1u<<24,XHCI_TRB_TYPE(33)|(outslot<<24)|m->ec};if(++m->head==63){assert(m->c->cmd[63].control==(XHCI_TRB_TYPE(6)|2|m->cycle));m->head=0;m->cycle^=1;}if(++m->ev==64){m->ev=0;m->ec^=1;}}
static int write32(void*p,uint32_t o,uint32_t v){struct model*m=p;if(o>=sizeof m->regs)return 0;m->regs[o/4]=v;if(o==OP){if(v==2){m->regs[o/4]=0;m->regs[(OP+4)/4]=1;}else if(v==1)m->regs[(OP+4)/4]=0;else if(!v)m->regs[(OP+4)/4]=1;}if(o==DB)command(m);return 1;}
static void delay(void*p){(void)p;}
static void start(struct model*m,struct xhci_controller*c,struct xhci_dma*cd,struct xhci_device_dma*dd,unsigned cs){memset(m,0,sizeof *m);m->c=cd;m->d=dd;m->cycle=m->ec=1;m->regs[0]=0x01100040;m->regs[1]=0x02000108;m->regs[4]=1|(cs?4:0);m->regs[5]=DB;m->regs[6]=RT;m->regs[(OP+4)/4]=1;m->regs[(OP+8)/4]=1;*c=(struct xhci_controller){0};struct xhci_io io={m,read32,write32,delay,sizeof m->regs};assert(xhci_controller_start(c,&io,cd,CDMA));}
int main(void){_Alignas(64) struct xhci_dma cd;_Alignas(64) struct xhci_device_dma dd;_Alignas(64) struct xhci_ep0_dma ed;struct model m;struct xhci_controller c;struct xhci_device d;assert(!xhci_address_context(&dd,16,1,3,64,EDMA));assert(!xhci_address_context(&dd,32,1,2,64,EDMA));assert(!xhci_address_context(&dd,64,1,3,8,EDMA));assert(xhci_address_context(&dd,32,1,1,8,EDMA));for(unsigned cs=0;cs<2;cs++){start(&m,&c,&cd,&dd,cs);assert(!xhci_device_address(&d,&c,&dd,EDMA,&ed,EDMA,1,3,64));assert(!xhci_device_address(&d,&c,&dd,DDMA,&ed,EDMA,3,3,64));assert(!m.commands);for(unsigned n=0;n<2000;n++){assert(xhci_device_address(&d,&c,&dd,DDMA,&ed,EDMA,1,3,64));assert(c.slot_state==2&&d.addressed&&cd.dcbaa[1]);assert(xhci_device_disable(&d)&&!cd.dcbaa[1]&&!c.slot_state&&!d.addressed&&d.ep0.unusable);}assert(m.commands==6000);assert(xhci_controller_stop(&c));}for(unsigned mode=1;mode<=2;mode++){start(&m,&c,&cd,&dd,0);m.mode=mode;assert(!xhci_device_address(&d,&c,&dd,DDMA,&ed,EDMA,1,3,64)&&c.state==3&&!d.addressed);if(mode==2)assert(cd.dcbaa[1]);}puts("XHCI_DEVICE_HOST_OK: 4000 slot enable/address/disable lifecycles, 32/64-byte contexts, DMA overlap and invalid slot/output rejection");}
