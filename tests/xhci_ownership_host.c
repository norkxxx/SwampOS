#include "../drivers/usb/xhci/ownership.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#define EXT 0x900
#define BIOS (1u<<16)
#define OS (1u<<24)
struct model{uint32_t r[1024];unsigned elapsed,writes,wait,fail;};
static int rd(void*p,uint32_t off,uint32_t*v){struct model*m=p;if(off>=sizeof m->r||off&3)return 0;*v=m->r[off/4];return 1;}
static int wr8(void*p,uint32_t off,uint8_t v){struct model*m=p;assert(off==EXT+3&&v==1);m->writes++;if(m->fail)return 0;m->r[EXT/4]|=OS;return 1;}
static void delay(void*p){struct model*m=p;m->elapsed++;if(m->elapsed==m->wait)m->r[EXT/4]&=~BIOS;}
static void init(struct model*m){memset(m,0,sizeof *m);m->r[0]=0x01100040;m->r[1]=0x02000108;m->r[4]=1|((EXT/4)<<16);m->r[5]=0x800;m->r[6]=0x600;m->r[EXT/4]=1|BIOS|(2u<<8);m->r[EXT/4+2]=2;m->wait=1;}
int main(void){struct model m;struct xhci_io io={&m,rd,0,delay,sizeof m.r};for(unsigned i=0;i<5000;i++){init(&m);m.wait=i%1000+1;assert(xhci_legacy_claim(&io,wr8));assert(m.elapsed==m.wait&&m.writes==1&&(m.r[EXT/4]&OS)&&!(m.r[EXT/4]&BIOS));}
 init(&m);m.wait=1001;assert(!xhci_legacy_claim(&io,wr8)&&m.elapsed==1000&&(m.r[EXT/4]&BIOS)&&(m.r[EXT/4]&OS));init(&m);m.r[EXT/4+1]=1;assert(!xhci_legacy_claim(&io,wr8)&&m.writes==1);init(&m);m.r[EXT/4]=1|OS;assert(xhci_legacy_claim(&io,wr8)&&!m.writes);init(&m);m.r[EXT/4+2]=1;assert(!xhci_legacy_claim(&io,wr8)&&!m.writes);init(&m);m.r[EXT/4]=1|(1u<<8);assert(!xhci_legacy_claim(&io,wr8)&&!m.writes);init(&m);m.r[4]=1;assert(xhci_legacy_claim(&io,wr8)&&!m.writes);init(&m);m.r[4]=1|((0x440/4)<<16);m.r[0x440/4]=1;assert(!xhci_legacy_claim(&io,wr8)&&!m.writes);init(&m);m.fail=1;assert(!xhci_legacy_claim(&io,wr8)&&m.writes==1);init(&m);assert(!xhci_legacy_claim(&io,0)&&!m.writes);
 puts("XHCI_OWNERSHIP_HOST_OK: 5000 cooperative firmware handoffs, timeout, SMI gate, malformed chains, byte-only ownership writes");}
