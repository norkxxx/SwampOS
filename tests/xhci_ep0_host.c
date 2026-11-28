#include "../drivers/usb/xhci/control_transport.h"
#include "../drivers/usb/core/enumeration.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#define OP 0x40
#define RT 0x600
#define DB 0x800
#define CDMA 0x100000
#define EDMA 0x200000
static const uint8_t dev[]={18,1,0,2,0,0,0,64,0x34,0x12,0x78,0x56,0,1,0,0,0,1};
static const uint8_t cfg[]={9,2,41,0,2,1,0,0x80,50,9,4,0,0,1,3,1,1,0,7,5,0x81,3,8,0,10,9,4,1,0,1,3,1,2,0,7,5,0x82,3,3,0,5};
struct mock{uint32_t reg[1024];struct xhci_dma *c;struct xhci_ep0_dma *e;unsigned idx,cycle,ev,ec,mode,delay,configured,protocol,transfers;};
static int rd(void*p,uint32_t o,uint32_t*v){struct mock*m=p;if(o>=sizeof m->reg)return 0;*v=m->reg[o/4];return 1;}
static struct xhci_trb take(struct mock*m,unsigned typ,uint64_t*addr){unsigned i=m->idx;struct xhci_trb t=m->e->ring[i];assert((t.control&1)==m->cycle&&((t.control>>10)&63)==typ);*addr=EDMA+i*16;if(++m->idx==63){assert(m->e->ring[63].control==(XHCI_TRB_TYPE(6)|2|m->cycle));m->idx=0;m->cycle^=1;}return t;}
static void emit(struct mock*m,uint64_t addr,unsigned code,unsigned resid,unsigned slot){m->c->event[m->ev]=(struct xhci_trb){(uint32_t)addr,(uint32_t)(addr>>32),(code<<24)|resid,XHCI_TRB_TYPE(32)|(1u<<16)|(slot<<24)|m->ec};if(++m->ev==64){m->ev=0;m->ec^=1;}}
static void transfer(struct mock*m){uint64_t addr,data=0;struct xhci_trb s=take(m,2,&addr);assert(s.status==8&&(s.control&(1u<<6)));unsigned type=s.lo&255,req=s.lo>>8&255,value=s.lo>>16,idx=s.hi&65535,n=s.hi>>16;unsigned in=!!(type&128);m->transfers++;if(n){struct xhci_trb d=take(m,3,&data);assert(d.lo==EDMA+offsetof(struct xhci_ep0_dma,bounce)&&!d.hi&&d.status==n);if(in){const uint8_t*src=(value>>8)==1?dev:cfg;unsigned max=(value>>8)==1?sizeof dev:sizeof cfg;assert(req==6&&n<=max);memcpy(m->e->bounce,src,n);}else assert(req==0x55&&m->e->bounce[0]==0xab);}else if(req==9){assert(!type&&value==1);m->configured=1;}else {assert(type==0x21&&req==0x0b&&!value&&idx<2&&m->configured);m->protocol|=1u<<idx;}
 struct xhci_trb st=take(m,4,&addr);assert(!!(st.control&(1u<<16))==(!n||!in));if(m->mode==1&&n&&in)emit(m,data,13,1,1);if(m->mode!=2&&m->mode!=3)emit(m,addr,1,0,m->mode==4?2:1);}
static int wr(void*p,uint32_t o,uint32_t v){struct mock*m=p;if(o>=sizeof m->reg)return 0;m->reg[o/4]=v;if(o==OP){if(v==2){m->reg[o/4]=0;m->reg[(OP+4)/4]=1;}else if(v==1)m->reg[(OP+4)/4]=0;else if(!v&&m->mode!=3)m->reg[(OP+4)/4]=1;}if(o==DB+4){assert(v==1);transfer(m);}return 1;}
static void delay(void*p){((struct mock*)p)->delay++;}
static void init(struct mock*m,struct xhci_dma*c,struct xhci_ep0_dma*e){memset(m,0,sizeof *m);m->c=c;m->e=e;m->cycle=m->ec=1;m->reg[0]=0x01100040;m->reg[1]=0x02000108;m->reg[4]=1;m->reg[5]=DB;m->reg[6]=RT;m->reg[(OP+4)/4]=1;m->reg[(OP+8)/4]=1;}
static void start(struct mock*m,struct xhci_controller*c,struct xhci_ep0*e,struct xhci_dma*cd,struct xhci_ep0_dma*ed){init(m,cd,ed);*c=(struct xhci_controller){0};struct xhci_io io={m,rd,wr,delay,sizeof m->reg};assert(xhci_controller_start(c,&io,cd,CDMA));assert(!xhci_ep0_init(e,c,ed,CDMA));assert(xhci_ep0_init(e,c,ed,EDMA));}
int main(void){_Alignas(64) struct xhci_dma cd;_Alignas(64) struct xhci_ep0_dma ed;struct mock m;struct xhci_controller c;struct xhci_ep0 e;start(&m,&c,&e,&cd,&ed);uint8_t b[4096];struct usb_enumerated u;assert(usb_enumerate_boot(xhci_ep0_transfer,&e,b,sizeof b,&u)==1&&u.config.count==2&&m.protocol==3&&m.transfers==6);uint8_t s[8]={0x80,6,0,1,0,0,18,0};size_t n;for(unsigned i=0;i<10000;i++){memset(b,0x55,18);assert(xhci_ep0_transfer(&e,s,b,18,&n)&&n==18&&!memcmp(b,dev,18));for(unsigned j=0;j<sizeof ed.bounce;j++)assert(!ed.bounce[j]);}s[0]=0x40;s[1]=0x55;s[6]=1;b[0]=0xab;assert(xhci_ep0_transfer(&e,s,b,1,&n)&&n==1);s[0]=0x80;s[1]=6;s[6]=18;m.mode=1;memset(b,0x55,18);assert(xhci_ep0_transfer(&e,s,b,18,&n)&&n==17&&b[17]==0x55);assert(xhci_controller_stop(&c));
 for(unsigned mode=2;mode<=4;mode++){start(&m,&c,&e,&cd,&ed);m.mode=mode;memset(b,0x55,18);assert(!xhci_ep0_transfer(&e,s,b,18,&n)&&!n&&e.unusable&&b[0]==0x55);assert(c.state==(mode==3?1u:3u));if(mode==3)assert(ed.bounce[0]==18&&m.delay==2000);else assert(!ed.bounce[0]);assert(!xhci_ep0_transfer(&e,s,b,18,&n));}puts("XHCI_EP0_HOST_OK: integrated enumeration of 2 HID interfaces, 10000 read transactions, OUT and short IN, fault/timeout, verified DMA retention on failed halt");}
