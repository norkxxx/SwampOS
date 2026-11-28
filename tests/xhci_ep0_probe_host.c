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
struct model {uint32_t regs[1024];struct xhci_dma*c;struct xhci_device_dma*d;unsigned head,cycle,ev,ec,slot,mode,commands;struct xhci_ep0_dma*ep;unsigned idx,tc,mps,speed,probes;};
static int read32(void*p,uint32_t o,uint32_t*v){struct model*m=p;if(o>=sizeof m->regs)return 0;*v=m->regs[o/4];return 1;}
static void command(struct model*m){unsigned idx=m->head;struct xhci_trb t=m->c->cmd[idx];unsigned type=(t.control>>10)&63,slot=t.control>>24;assert((t.control&1)==m->cycle);m->commands++;if(type==9){assert(!slot&&!m->slot&&!t.lo&&!t.hi);m->slot=1;}else if(type==11){assert(slot==1&&m->slot==1&&t.lo==DDMA&&!t.hi);assert(m->c->dcbaa[1]==DDMA+offsetof(struct xhci_device_dma,output));unsigned stride=(m->regs[4]&4)?16:8;assert(m->d->input[1]==3&&m->d->input[stride]==((m->speed<<20)|(1u<<27))&&m->d->input[stride+1]==1u<<16&&m->d->input[stride*2+2]==(EDMA|1));m->d->output[0]=m->d->input[stride];m->d->output[3]=(2u<<27)|7;m->d->output[stride]=1;m->d->output[stride+1]=m->d->input[stride*2+1];m->slot=2;}else if(type==13){unsigned stride=(m->regs[4]&4)?16:8;assert(slot==1&&m->slot==2&&t.lo==DDMA&&m->d->input[1]==2);assert(m->d->input[2*stride+1]==m->mps<<16);if(m->mode!=4)m->d->output[stride+1]=(m->d->output[stride+1]&0xffff)|(m->mps<<16);}else if(type==10){assert(slot==1&&m->slot==2&&m->c->dcbaa[1]);m->slot=0;}else assert(0);if(m->mode==2&&type==11)m->d->output[3]=0;unsigned outslot=m->mode==1?2:1;m->c->event[m->ev]=(struct xhci_trb){CDMA+idx*16,0,1u<<24,XHCI_TRB_TYPE(33)|(outslot<<24)|m->ec};if(++m->head==63){assert(m->c->cmd[63].control==(XHCI_TRB_TYPE(6)|2|m->cycle));m->head=0;m->cycle^=1;}if(++m->ev==64){m->ev=0;m->ec^=1;}}
static void probe(struct model*m){unsigned idx=m->idx;struct xhci_trb s=m->ep->ring[idx],d=m->ep->ring[idx+1],t=m->ep->ring[idx+2];assert((s.control>>10&63)==2&&s.lo==0x01000680&&s.hi==8u<<16);assert((d.control>>10&63)==3&&d.status==8&&(t.control>>10&63)==4);memset(m->ep->bounce,0,8);m->ep->bounce[0]=18;m->ep->bounce[1]=1;m->ep->bounce[7]=m->mps;m->c->event[m->ev]=(struct xhci_trb){EDMA+(idx+2)*16,0,1u<<24,XHCI_TRB_TYPE(32)|(1u<<24)|(1u<<16)|m->ec};m->idx+=3;if(++m->ev==64){m->ev=0;m->ec^=1;}m->probes++;}
static int write32(void*p,uint32_t o,uint32_t v){struct model*m=p;if(o>=sizeof m->regs)return 0;m->regs[o/4]=v;if(o==OP){if(v==2){m->regs[o/4]=0;m->regs[(OP+4)/4]=1;}else if(v==1)m->regs[(OP+4)/4]=0;else if(!v)m->regs[(OP+4)/4]=1;}if(o==DB)command(m);if(o==DB+4){assert(v==1);probe(m);}return 1;}
static void delay(void*p){(void)p;}
static void start(struct model*m,struct xhci_controller*c,struct xhci_dma*cd,struct xhci_device_dma*dd,unsigned cs){memset(m,0,sizeof *m);m->c=cd;m->d=dd;m->cycle=m->ec=1;m->regs[0]=0x01100040;m->regs[1]=0x02000108;m->regs[4]=1|(cs?4:0);m->regs[5]=DB;m->regs[6]=RT;m->regs[(OP+4)/4]=1;m->regs[(OP+8)/4]=1;*c=(struct xhci_controller){0};struct xhci_io io={m,read32,write32,delay,sizeof m->regs};assert(xhci_controller_start(c,&io,cd,CDMA));}
int main(void){_Alignas(64) struct xhci_dma cd;_Alignas(64) struct xhci_device_dma dd;_Alignas(64) struct xhci_ep0_dma ed;struct model m;struct xhci_controller c;struct xhci_device d;
 for(unsigned i=0;i<4000;i++){start(&m,&c,&cd,&dd,i&1);m.ep=&ed;m.speed=1;m.mps=8u<<(i%4);assert(xhci_device_address(&d,&c,&dd,DDMA,&ed,EDMA,1,1,8));assert(xhci_device_probe_ep0(&d)&&d.ep0_packet==m.mps&&m.probes==1);assert(m.commands==2+(m.mps!=8));assert(xhci_device_disable(&d)&&xhci_controller_stop(&c));}
 for(unsigned i=0;i<3;i++){start(&m,&c,&cd,&dd,0);m.ep=&ed;m.speed=i==1?3:1;m.mps=i==0?7:i==1?8:64;assert(xhci_device_address(&d,&c,&dd,DDMA,&ed,EDMA,1,m.speed,m.speed==3?64:8));if(i==2)m.mode=4;assert(!xhci_device_probe_ep0(&d)&&c.state==3);}
 puts("XHCI_EP0_PROBE_HOST_OK: 4000 full-speed MPS probes, 8/16/32/64, both context sizes, invalid size and failed context update");}
