#include "../drivers/usb/xhci/boot_session.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#define OP 0x40
#define RT 0x600
#define DB 0x800
#define CDMA 0x100000
#define DDMA 0x200000
#define EDMA 0x300000
#define EXT 0x900
#define PORT (OP+0x400)
#define PP 512u
#define PR 16u
#define PRC (1u<<21)
struct model {uint32_t regs[1024];struct xhci_dma*c;struct xhci_device_dma*d;unsigned head,cycle,ev,ec,slot,mode,commands;struct xhci_hid_dma*h;struct xhci_hid*client;unsigned reports;struct xhci_ep0_dma*ep;unsigned idx,tc,configured,protocol,elapsed,reset,speed;};
static int read32(void*p,uint32_t o,uint32_t*v){struct model*m=p;if(o>=sizeof m->regs)return 0;*v=m->regs[o/4];return 1;}
static void command(struct model*m){unsigned idx=m->head;struct xhci_trb t=m->c->cmd[idx];unsigned type=(t.control>>10)&63,slot=t.control>>24;assert((t.control&1)==m->cycle);m->commands++;if(type==9){assert(!slot&&!m->slot&&!t.lo&&!t.hi);m->slot=1;}else if(type==11){assert(slot==1&&m->slot==1&&t.lo==DDMA&&!t.hi);assert(m->c->dcbaa[1]==DDMA+offsetof(struct xhci_device_dma,output));unsigned stride=(m->regs[4]&4)?16:8;assert(m->d->input[1]==3&&m->d->input[stride]==((m->speed<<20)|(1u<<27))&&m->d->input[stride+1]==1u<<16&&m->d->input[stride*2+2]==(EDMA|1));m->d->output[0]=m->d->input[stride];m->d->output[1]=m->d->input[stride+1];m->d->output[3]=(2u<<27)|7;m->d->output[stride]=1;m->d->output[stride+1]=m->d->input[2*stride+1];m->slot=2;}else if(type==13){unsigned n=(m->regs[4]&4)?16:8;assert(m->d->input[1]==2&&m->d->input[2*n+1]==64u<<16);m->d->output[n+1]=(m->d->output[n+1]&0xffff)|(64u<<16);}else if(type==12){unsigned n=(m->regs[4]&4)?16:8;assert(slot==1&&m->slot==2&&t.lo==DDMA&&m->configured&&m->protocol);assert(m->d->input[1]==(1u|1u<<3));assert(m->d->input[4*n+2]==0x400001);m->d->output[3]=(3u<<27)|7;m->d->output[3*n]=1;}else if(type==10){assert(slot==1&&m->slot==2&&m->c->dcbaa[1]);m->slot=0;}else assert(0);if(m->mode==2&&type==11)m->d->output[3]=0;unsigned outslot=m->mode==1?2:1;m->c->event[m->ev]=(struct xhci_trb){CDMA+idx*16,0,1u<<24,XHCI_TRB_TYPE(33)|(outslot<<24)|m->ec};if(++m->head==63){assert(m->c->cmd[63].control==(XHCI_TRB_TYPE(6)|2|m->cycle));m->head=0;m->cycle^=1;}if(++m->ev==64){m->ev=0;m->ec^=1;}}
static struct xhci_trb take(struct model*m,unsigned type,uint64_t*addr){unsigned i=m->idx;struct xhci_trb t=m->ep->ring[i];assert((t.control&1)==m->tc&&((t.control>>10)&63)==type);*addr=EDMA+i*16;if(++m->idx==63){m->idx=0;m->tc^=1;}return t;}
static void ep0(struct model*m){static const uint8_t dev[]={18,1,0,2,0,0,0,64,0x34,0x12,0x78,0x56,0,1,0,0,0,1};uint8_t cfg[]={9,2,25,0,1,1,0,0x80,50,9,4,0,0,1,3,1,1,0,7,5,0x81,3,8,0,4};cfg[16]=m->client->protocol==2?2:1; /* session has not prepared HID yet; keyboard default */
 uint64_t addr;struct xhci_trb t=take(m,2,&addr);unsigned req=t.lo>>8&255,value=t.lo>>16,n=t.hi>>16;
 if(n){struct xhci_trb d=take(m,3,&addr);assert(d.status==n);const uint8_t*src=(value>>8)==1?dev:cfg;assert(req==6&&n<=((value>>8)==1?sizeof dev:sizeof cfg));memcpy(m->ep->bounce,src,n);}else if(req==9)m->configured=1;else if(req==11&&m->configured)m->protocol=1;else assert(0);
 (void)take(m,4,&addr);m->c->event[m->ev]=(struct xhci_trb){(uint32_t)addr,0,1u<<24,XHCI_TRB_TYPE(32)|(1u<<24)|(1u<<16)|m->ec};if(++m->ev==64){m->ev=0;m->ec^=1;}}
static void report(struct model*m,unsigned dci){struct xhci_interrupt_ring*r=&m->client->ring;assert(dci==3&&r->state==1);unsigned idx=(unsigned)(r->pending-r->dma)/16;struct xhci_trb t=m->h->ring[idx];assert(t.lo==0x400000+offsetof(struct xhci_hid_dma,report)&&t.status==r->packet);if(m->mode==1||m->mode==2)return;
 unsigned len=r->packet;memset(m->h->report,0,64);if(m->client->protocol==1){m->h->report[2]=(m->reports&1)?0:4;len=8;}else{m->h->report[0]=1;m->h->report[1]=255;m->h->report[2]=2;len=3;}
 if(m->mode==4)len=1;
 m->c->event[m->ev]=(struct xhci_trb){(uint32_t)r->pending,0,((len==r->packet?1u:13u)<<24)|(r->packet-len),XHCI_TRB_TYPE(m->mode==5?34:32)|(1u<<24)|(3u<<16)|m->ec};if(++m->ev==64){m->ev=0;m->ec^=1;}m->reports++;}
static int write32(void*p,uint32_t o,uint32_t v){struct model*m=p;if(o>=sizeof m->regs)return 0;m->regs[o/4]=v;if(o==OP){if(v==2){m->regs[o/4]=0;m->regs[(OP+4)/4]=1;}else if(v==1)m->regs[(OP+4)/4]=0;else if(!v&&m->mode!=2)m->regs[(OP+4)/4]=1;}if(o==PORT){m->regs[o/4]=1|PP|(m->speed<<10);if(v&PR){m->reset=m->elapsed;m->regs[o/4]|=PR;}else m->regs[o/4]|=2;}if(o==DB)command(m);if(o==DB+4){if(v==1)ep0(m);else report(m,v);}return 1;}
static void delay(void*p){struct model*m=p;m->elapsed++;if(m->reset&&m->elapsed-m->reset==20)m->regs[PORT/4]=1|2|PP|PRC|(m->speed<<10);}
static void init(struct model*m,struct xhci_boot_session*s,struct xhci_boot_buffers*b,unsigned speed,unsigned cs){memset(m,0,sizeof *m);memset(s,0,sizeof *s);m->c=b->c;m->d=b->d;m->ep=b->ep;m->h=b->h;m->client=&s->hid;m->speed=speed;m->cycle=m->ec=m->tc=1;m->regs[0]=0x01100040;m->regs[1]=0x02000108;m->regs[4]=1|8|(cs?4:0)|((EXT/4)<<16);m->regs[5]=DB;m->regs[6]=RT;m->regs[(OP+4)/4]=1;m->regs[(OP+8)/4]=1;m->regs[EXT/4]=(2u<<24)|2;m->regs[EXT/4+1]=0x20425355;m->regs[EXT/4+2]=0x0101;m->regs[PORT/4]=1|PP;}
int main(void){_Alignas(64) struct xhci_dma cd;_Alignas(64) struct xhci_device_dma dd;_Alignas(64) struct xhci_ep0_dma ed;_Alignas(64) struct xhci_hid_dma hd;struct xhci_boot_buffers b={&cd,&dd,&ed,&hd,CDMA,DDMA,EDMA,0x400000};struct model m;struct xhci_boot_session s;struct xhci_io io={&m,read32,write32,delay,sizeof m.regs};struct xhci_hid_event out;
 for(unsigned i=0;i<1000;i++){init(&m,&s,&b,i&1?1:3,i&1);assert(xhci_boot_start(&s,&io,&b,1)&&s.state==1&&s.usb.vendor==0x1234&&s.device.ep0_packet==64);assert(m.commands==(i&1?4u:3u));for(unsigned j=0;j<20;j++){assert(xhci_boot_poll(&s,&out)==1&&out.protocol==1&&out.keys.count==!(j&1));}assert(xhci_boot_stop(&s)&&s.state==2);assert(!xhci_boot_start(&s,&io,&b,1));}
 init(&m,&s,&b,3,0);struct xhci_boot_buffers bad=b;bad.hd=bad.ed;assert(!xhci_boot_start(&s,&io,&bad,1)&&!s.state&&!m.commands);assert(!xhci_boot_start(&s,&io,&b,2)&&!s.state);
 init(&m,&s,&b,3,0);assert(xhci_boot_start(&s,&io,&b,1));m.mode=2;assert(!xhci_boot_stop(&s)&&s.state==3);m.mode=0;assert(xhci_boot_stop(&s)&&s.state==2);
 puts("XHCI_BOOT_SESSION_HOST_OK: 1000 end-to-end sessions, 20000 reports, USB2 reset, slot, MPS, enumeration, endpoint configuration, halt and quarantine");}
