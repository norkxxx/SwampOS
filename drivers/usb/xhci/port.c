#include "port.h"
#define CCS (1u<<0)
#define PED (1u<<1)
#define OCA (1u<<3)
#define PR (1u<<4)
#define PP (1u<<9)
#define PRC (1u<<21)
#define HSE (1u<<2)
#define HCE (1u<<12)
/* Do not echo RW1C change bits, PED, LWS, PR or warm reset from PORTSC. */
#define PORT_PRESERVE (PP|(3u<<14)|(7u<<25))
static int rd(const struct xhci_controller*c,uint32_t off,uint32_t*v){const struct xhci_io*i=&c->io;return i->read32&&i->aperture>=4&&!(off&3)&&off<=i->aperture-4&&i->read32(i->ctx,off,v);}
static int wr(struct xhci_controller*c,uint32_t off,uint32_t v){const struct xhci_io*i=&c->io;return i->write32&&i->aperture>=4&&!(off&3)&&off<=i->aperture-4&&i->write32(i->ctx,off,v);}
int xhci_usb2_port(const struct xhci_controller*c,unsigned port){if(!c||!port||port>c->caps.ports||!c->caps.ext)return 0;uint32_t off=c->caps.ext;unsigned hits=0,ok=0;for(unsigned i=0;i<256;i++){uint32_t a;if(off<0x20||!rd(c,off,&a)||!(a&255)||(a&255)==255)return 0;unsigned next=(a>>8)&255;if((a&255)==2){uint32_t name,info,slot;if(off>UINT32_MAX-12||!rd(c,off+4,&name)||!rd(c,off+8,&info)||!rd(c,off+12,&slot))return 0;unsigned first=info&255,count=(info>>8)&255,psi=info>>28;if(name!=0x20425355u||!first||!count||first>c->caps.ports||count>c->caps.ports-first+1||(next&&next<4+psi)||off>UINT32_MAX-(12+psi*4)||c->io.aperture<4||off+12+psi*4>c->io.aperture-4)return 0;if(port>=first&&port<first+count){hits++;ok=(a>>24)==2&&!psi&&!(slot&31);}}if(!next)return hits==1&&ok;if(off>UINT32_MAX-next*4)return 0;off+=next*4;}return 0;}
static int port_read(struct xhci_controller*c,uint32_t off,uint32_t*v){uint32_t status;return rd(c,c->caps.op+4,&status)&&!(status&(1|HSE|HCE))&&rd(c,off,v)&&(*v&CCS)&&!(*v&OCA);}
unsigned xhci_port_reset(struct xhci_controller*c,unsigned port){if(!c||c->state!=2||xhci_slots_full(c)||c->cmd.count||!c->io.delay_ms||!xhci_usb2_port(c,port))return 0;uint32_t off=c->caps.op+0x400+(port-1)*16,v;if(!port_read(c,off,&v)||v&PR)return 0;if(!(v&PP)){if(!c->caps.ppc||!wr(c,off,(v&PORT_PRESERVE)|PP))return 0;for(unsigned j=0;j<20;j++)c->io.delay_ms(c->io.ctx);}
 /* 100 ms of connected, powered, non-resetting state. */
 for(unsigned j=0;j<100;j++){if(!port_read(c,off,&v)||!(v&PP)||(v&PR))return 0;c->io.delay_ms(c->io.ctx);}if(!port_read(c,off,&v)||!(v&PP)||(v&PR))return 0;
 if(v&PRC){if(!wr(c,off,(v&PORT_PRESERVE)|PRC)||!port_read(c,off,&v)||(v&PRC))return 0;}
 if(!wr(c,off,(v&PORT_PRESERVE)|PR))return 0;
 for(unsigned j=0;j<1000;j++){if(!port_read(c,off,&v)||!(v&PP))return 0;if(!(v&PR)&&(v&PRC)){unsigned speed=(v>>10)&15;if(!(v&PED)||speed<1||speed>3)return 0;if(!wr(c,off,(v&PORT_PRESERVE)|PRC))return 0;return speed;}c->io.delay_ms(c->io.ctx);}return 0;}
