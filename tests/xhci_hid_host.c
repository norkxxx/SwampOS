#include "../drivers/usb/xhci/hid_transport.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#define OP 0x40
#define RT 0x600
#define DB 0x800
#define CDMA 0x100000
#define DDMA 0x200000
#define EDMA 0x300000
struct model {uint32_t regs[1024];struct xhci_dma*c;struct xhci_device_dma*d;unsigned head,cycle,ev,ec,slot,mode,commands;struct xhci_hid_dma*h;struct xhci_hid*client;unsigned reports;};
static int read32(void*p,uint32_t o,uint32_t*v){struct model*m=p;if(o>=sizeof m->regs)return 0;*v=m->regs[o/4];return 1;}
static void command(struct model*m){unsigned idx=m->head;struct xhci_trb t=m->c->cmd[idx];unsigned type=(t.control>>10)&63,slot=t.control>>24;assert((t.control&1)==m->cycle);m->commands++;if(type==9){assert(!slot&&!m->slot&&!t.lo&&!t.hi);m->slot=1;}else if(type==11){assert(slot==1&&m->slot==1&&t.lo==DDMA&&!t.hi);assert(m->c->dcbaa[1]==DDMA+offsetof(struct xhci_device_dma,output));unsigned stride=(m->regs[4]&4)?16:8;assert(m->d->input[1]==3&&m->d->input[stride]==((3u<<20)|(1u<<27))&&m->d->input[stride+1]==1u<<16&&m->d->input[stride*2+2]==(EDMA|1));m->d->output[0]=m->d->input[stride];m->d->output[1]=m->d->input[stride+1];m->d->output[3]=(2u<<27)|7;m->slot=2;}else if(type==12){unsigned n=(m->regs[4]&4)?16:8;assert(slot==1&&m->slot==2&&t.lo==DDMA);assert(m->d->input[1]==(1u|1u<<3));assert(m->d->input[4*n+2]==0x400001);m->d->output[3]=(3u<<27)|7;m->d->output[3*n]=1;}else if(type==10){assert(slot==1&&m->slot==2&&m->c->dcbaa[1]);m->slot=0;}else assert(0);if(m->mode==2&&type==11)m->d->output[3]=0;unsigned outslot=m->mode==1?2:1;m->c->event[m->ev]=(struct xhci_trb){CDMA+idx*16,0,1u<<24,XHCI_TRB_TYPE(33)|(outslot<<24)|m->ec};if(++m->head==63){assert(m->c->cmd[63].control==(XHCI_TRB_TYPE(6)|2|m->cycle));m->head=0;m->cycle^=1;}if(++m->ev==64){m->ev=0;m->ec^=1;}}
static void report(struct model*m,unsigned dci){struct xhci_interrupt_ring*r=&m->client->ring;assert(dci==3&&r->state==1);unsigned idx=(unsigned)(r->pending-r->dma)/16;struct xhci_trb t=m->h->ring[idx];assert(t.lo==0x400000+offsetof(struct xhci_hid_dma,report)&&t.status==r->packet);if(m->mode==1||m->mode==2)return;
 unsigned len=r->packet;memset(m->h->report,0,64);if(m->client->protocol==1){m->h->report[2]=(m->reports&1)?0:4;len=8;}else{m->h->report[0]=1;m->h->report[1]=255;m->h->report[2]=2;len=3;}
 if(m->mode==4)len=1;
 m->c->event[m->ev]=(struct xhci_trb){(uint32_t)r->pending,0,((len==r->packet?1u:13u)<<24)|(r->packet-len),XHCI_TRB_TYPE(m->mode==5?34:32)|(1u<<24)|(3u<<16)|m->ec};if(++m->ev==64){m->ev=0;m->ec^=1;}m->reports++;}
static int write32(void*p,uint32_t o,uint32_t v){struct model*m=p;if(o>=sizeof m->regs)return 0;m->regs[o/4]=v;if(o==OP){if(v==2){m->regs[o/4]=0;m->regs[(OP+4)/4]=1;}else if(v==1)m->regs[(OP+4)/4]=0;else if(!v&&m->mode!=2)m->regs[(OP+4)/4]=1;}if(o==DB)command(m);if(o==DB+4)report(m,v);return 1;}
static void delay(void*p){(void)p;}
static void start(struct model*m,struct xhci_controller*c,struct xhci_dma*cd,struct xhci_device_dma*dd,unsigned cs){memset(m,0,sizeof *m);m->c=cd;m->d=dd;m->cycle=m->ec=1;m->regs[0]=0x01100040;m->regs[1]=0x02000108;m->regs[4]=1|(cs?4:0);m->regs[5]=DB;m->regs[6]=RT;m->regs[(OP+4)/4]=1;m->regs[(OP+8)/4]=1;*c=(struct xhci_controller){0};struct xhci_io io={m,read32,write32,delay,sizeof m->regs};assert(xhci_controller_start(c,&io,cd,CDMA));}
int main(void){_Alignas(64) struct xhci_dma cd;_Alignas(64) struct xhci_device_dma dd;_Alignas(64) struct xhci_ep0_dma ed;_Alignas(64) struct xhci_hid_dma hd;struct model m;struct xhci_controller c;struct xhci_device d;struct xhci_hid h;struct xhci_hid_event out;struct xhci_interrupt_spec spec[8];uint64_t ring[]={0x400000};
 for(unsigned protocol=1;protocol<=2;protocol++){start(&m,&c,&cd,&dd,0);assert(xhci_device_address(&d,&c,&dd,DDMA,&ed,EDMA,1,3,64));struct usb_boot_config cfg={.configuration=1,.count=1,.interfaces={{0,protocol,0x81,4,8}}};m.h=&hd;m.client=&h;
 assert(!xhci_hid_prepare(&h,&d,&hd,DDMA,cfg.interfaces));assert(xhci_hid_prepare(&h,&d,&hd,ring[0],cfg.interfaces));assert(!xhci_hid_submit(&h));assert(xhci_configure_interrupts(&d,&cfg,ring,spec));
 for(unsigned i=0;i<10000;i++){assert(xhci_hid_submit(&h));assert(!xhci_hid_submit(&h));assert(xhci_hid_poll(&h,&out)==1&&out.protocol==protocol);if(protocol==1)assert(out.keys.count==!(i&1)&&(!(i&1)?out.keys.keys[0]=='a':1));else assert(out.mouse.dx==-1&&out.mouse.dy==2&&out.mouse.buttons==1);for(unsigned j=0;j<64;j++)assert(!hd.report[j]);}
 m.mode=4;assert(xhci_hid_submit(&h)&&xhci_hid_poll(&h,&out)==0&&!h.ring.state&&!out.protocol);m.mode=1;assert(xhci_hid_submit(&h));for(unsigned j=0;j<50;j++)assert(xhci_hid_poll(&h,&out)==0&&h.ring.state==1);if(protocol==1){m.mode=2;hd.report[0]=99;assert(!xhci_hid_abort(&h)&&c.state==1&&hd.report[0]==99);}else assert(xhci_hid_abort(&h)&&c.state==3&&!hd.report[0]);assert(!xhci_hid_submit(&h));}
 puts("XHCI_HID_HOST_OK: 20000 modeled keyboard/mouse reports, short and malformed reports, nonblocking idle, abort and DMA quarantine");}
