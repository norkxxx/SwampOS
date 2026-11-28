#include "../drivers/usb/xhci/controller.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#define OP 0x40
#define RT 0x600
#define DB 0x800
#define DMA 0x100000
struct mock {uint32_t reg[1024];struct xhci_dma *m;unsigned cmd,event,cc,ec,delays,mode,writes;};
static int read32(void*p,uint32_t o,uint32_t*v){struct mock*m=p;if(o>=sizeof m->reg)return 0;*v=m->reg[o/4];return 1;}
static int write32(void*p,uint32_t o,uint32_t v){struct mock*m=p;if(o>=sizeof m->reg)return 0;m->writes++;m->reg[o/4]=v;if(o==OP){if(v==2){if(m->mode!=1)m->reg[o/4]=0;m->reg[(OP+4)/4]=1;}else if(v==1)m->reg[(OP+4)/4]=0;else if(!v&&m->mode!=3)m->reg[(OP+4)/4]=1;}if(o==DB&&m->mode!=2&&m->mode!=3){unsigned q=m->cmd;assert(m->m->cmd[q].control==(XHCI_TRB_TYPE(23)|m->cc));uint64_t ptr=DMA+q*16;if(m->mode==4)ptr+=16;m->m->event[m->event]=(struct xhci_trb){(uint32_t)ptr,0,1u<<24,XHCI_TRB_TYPE(33)|m->ec};if(++m->cmd==63){assert(m->m->cmd[63].control==(XHCI_TRB_TYPE(6)|2|m->cc));m->cmd=0;m->cc^=1;}if(++m->event==64){m->event=0;m->ec^=1;}}return 1;}
static void delay(void*p){((struct mock*)p)->delays++;}
static void init(struct mock*m,struct xhci_dma*d){memset(m,0,sizeof *m);m->m=d;m->cc=m->ec=1;m->reg[0]=0x01100040;m->reg[1]=0x02000108;m->reg[4]=1;m->reg[5]=DB;m->reg[6]=RT;m->reg[(OP+4)/4]=1;m->reg[(OP+8)/4]=1;}
int main(void){_Alignas(64) struct xhci_dma d;struct mock m;struct xhci_controller c={0};init(&m,&d);struct xhci_io io={&m,read32,write32,delay,sizeof m.reg};struct xhci_caps cap;assert(xhci_caps_read(&io,&cap)&&cap.ports==2&&cap.slots==8);assert(xhci_controller_start(&c,&io,&d,DMA));assert(c.state==2);assert(m.reg[(OP+0x30)/4]==DMA+offsetof(struct xhci_dma,dcbaa));assert(d.erst[0]==DMA+offsetof(struct xhci_dma,event)&&d.erst[2]==64);for(unsigned n=0;n<10000;n++)assert(xhci_controller_noop(&c));assert(xhci_controller_stop(&c)&&c.state==3);assert(!xhci_controller_noop(&c));
 for(unsigned mode=1;mode<=4;mode++){init(&m,&d);c=(struct xhci_controller){0};m.mode=mode;if(mode==1){assert(!xhci_controller_start(&c,&io,&d,DMA)&&c.state==3&&m.delays==1000);}else{assert(xhci_controller_start(&c,&io,&d,DMA));assert(!xhci_controller_noop(&c));assert(c.state==(mode==3?1u:3u));if(mode==2)assert(m.delays==1000);if(mode==3)assert(m.delays==2000);}}
 init(&m,&d);c=(struct xhci_controller){0};m.reg[2]=1u<<27;assert(!xhci_controller_start(&c,&io,&d,DMA)&&!m.writes);m.reg[2]=0;m.reg[4]=0;assert(!xhci_controller_start(&c,&io,&d,0x100000000ull)&&!m.writes);m.reg[5]=RT+0x20;assert(!xhci_caps_read(&io,&cap));puts("XHCI_CONTROLLER_HOST_OK: 10000 doorbell/completion exchanges, reset/run/stop, timeout, invalid completion, DMA quarantine, capability rejection");}
