#include "../drivers/usb/xhci/interrupt_context.h"
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
static void command(struct model*m){unsigned idx=m->head;struct xhci_trb t=m->c->cmd[idx];unsigned type=(t.control>>10)&63,slot=t.control>>24;assert((t.control&1)==m->cycle);m->commands++;if(type==9){assert(!slot&&!m->slot&&!t.lo&&!t.hi);m->slot=1;}else if(type==11){assert(slot==1&&m->slot==1&&t.lo==DDMA&&!t.hi);assert(m->c->dcbaa[1]==DDMA+offsetof(struct xhci_device_dma,output));unsigned stride=(m->regs[4]&4)?16:8;assert(m->d->input[1]==3&&m->d->input[stride]==((3u<<20)|(1u<<27))&&m->d->input[stride+1]==1u<<16&&m->d->input[stride*2+2]==(EDMA|1));m->d->output[0]=m->d->input[stride];m->d->output[1]=m->d->input[stride+1];m->d->output[3]=(2u<<27)|7;m->slot=2;}else if(type==12){unsigned n=(m->regs[4]&4)?16:8;assert(slot==1&&m->slot==2&&t.lo==DDMA);assert(m->d->input[1]==(1u|1u<<3|1u<<31));assert(m->d->input[n]>>27==31);assert(m->d->input[4*n]==(3u<<16));assert(m->d->input[4*n+1]==(3u<<1|7u<<3|8u<<16));assert(m->d->input[4*n+2]==0x400001);assert(m->d->input[32*n+2]==0x410001);m->d->output[3]=(3u<<27)|7;m->d->output[3*n]=1;m->d->output[31*n]=m->mode==3?0:1;}else if(type==10){assert(slot==1&&m->slot==2&&m->c->dcbaa[1]);m->slot=0;}else assert(0);if(m->mode==2&&type==11)m->d->output[3]=0;unsigned outslot=m->mode==1?2:1;m->c->event[m->ev]=(struct xhci_trb){CDMA+idx*16,0,1u<<24,XHCI_TRB_TYPE(33)|(outslot<<24)|m->ec};if(++m->head==63){assert(m->c->cmd[63].control==(XHCI_TRB_TYPE(6)|2|m->cycle));m->head=0;m->cycle^=1;}if(++m->ev==64){m->ev=0;m->ec^=1;}}
static int write32(void*p,uint32_t o,uint32_t v){struct model*m=p;if(o>=sizeof m->regs)return 0;m->regs[o/4]=v;if(o==OP){if(v==2){m->regs[o/4]=0;m->regs[(OP+4)/4]=1;}else if(v==1)m->regs[(OP+4)/4]=0;else if(!v)m->regs[(OP+4)/4]=1;}if(o==DB)command(m);return 1;}
static void delay(void*p){(void)p;}
static void start(struct model*m,struct xhci_controller*c,struct xhci_dma*cd,struct xhci_device_dma*dd,unsigned cs){memset(m,0,sizeof *m);m->c=cd;m->d=dd;m->cycle=m->ec=1;m->regs[0]=0x01100040;m->regs[1]=0x02000108;m->regs[4]=1|(cs?4:0);m->regs[5]=DB;m->regs[6]=RT;m->regs[(OP+4)/4]=1;m->regs[(OP+8)/4]=1;*c=(struct xhci_controller){0};struct xhci_io io={m,read32,write32,delay,sizeof m->regs};assert(xhci_controller_start(c,&io,cd,CDMA));}
int main(void){
 _Alignas(64) struct xhci_dma cd;_Alignas(64) struct xhci_device_dma dd;_Alignas(64) struct xhci_ep0_dma ed;struct model m;struct xhci_controller c;struct xhci_device d;
 struct usb_boot_config cfg={.configuration=1,.count=2,.interfaces={{0,1,0x81,4,8},{1,2,0x8f,5,3}}};uint64_t rings[]={0x400000,0x410000};struct xhci_interrupt_spec s[8];
 for(unsigned cs=0;cs<2;cs++){start(&m,&c,&cd,&dd,cs);for(unsigned n=0;n<2000;n++){
 assert(xhci_device_address(&d,&c,&dd,DDMA,&ed,EDMA,1,3,64));
 uint64_t bad[]={0x400000,0x400040};assert(!xhci_configure_interrupts(&d,&cfg,bad,s));bad[1]=EDMA;assert(!xhci_configure_interrupts(&d,&cfg,bad,s));
 assert(xhci_configure_interrupts(&d,&cfg,rings,s)&&d.configured&&s[0].dci==3&&s[1].dci==31);assert(!xhci_configure_interrupts(&d,&cfg,rings,s));assert(xhci_device_disable(&d)&&!d.configured);
 }assert(m.commands==8000);assert(xhci_controller_stop(&c));}
 struct usb_boot_interface i=cfg.interfaces[0];for(unsigned n=1;n<256;n++){i.interval=n;assert(xhci_interrupt_spec(&i,1,0x400000,s));assert((1u<<s[0].interval)<=8*n&&2u*(1u<<s[0].interval)>8*n);assert(xhci_interrupt_spec(&i,3,0x400000,s)==(n<=16));}
 i.packet_size=9;assert(!xhci_interrupt_spec(&i,2,0x400000,s));i.packet_size=8;i.interval=1;assert(!xhci_interrupt_spec(&i,1,0x40ffc0,s));
 start(&m,&c,&cd,&dd,0);assert(xhci_device_address(&d,&c,&dd,DDMA,&ed,EDMA,1,3,64));m.mode=3;assert(!xhci_configure_interrupts(&d,&cfg,rings,s)&&c.state==3&&!d.configured);
 puts("XHCI_INTERRUPT_CONTEXT_HOST_OK: 4000 configure/disable lifecycles, 32/64-byte contexts, DCI 31, interval, overlap, failed endpoint state");}
