#include "ownership.h"
#define BIOS_OWNED (1u<<16)
#define OS_OWNED (1u<<24)
/* USBLEGCTLSTS interrupt enable bits, xHCI section 7.1.2. We only read them;
 * any enabled firmware SMI is a reason to reject the narrow backend. */
#define LEGACY_SMI_ENABLE (1u|(1u<<4)|(7u<<13))
static int rd(const struct xhci_io*i,uint32_t off,uint32_t*v){return i->read32&&i->aperture>=4&&!(off&3)&&off<=i->aperture-4&&i->read32(i->ctx,off,v)&&*v!=UINT32_MAX;}
static int overlap(uint32_t a,uint32_t n,uint32_t b,uint32_t m){return (uint64_t)a<(uint64_t)b+m&&(uint64_t)b<(uint64_t)a+n;}
int xhci_legacy_claim(const struct xhci_io*i,xhci_write8_fn write8){
 struct xhci_caps c;if(!i||!xhci_caps_read(i,&c))return 0;if(!c.ext)return 1;
 uint32_t off=c.ext,legacy=0,v;unsigned found=0;
 for(unsigned k=0;k<256;k++){
  if(off<0x20||!rd(i,off,&v)||!(v&255)||(v&255)==255)return 0;
  unsigned id=v&255,next=(v>>8)&255;
  if(id==1){if(found||off>UINT32_MAX-8||i->aperture<8||off>i->aperture-8||(next&&next<2)||overlap(off,8,c.op,0x400+16*c.ports)||overlap(off,8,c.rt,0x40)||overlap(off,8,c.db,8))return 0;legacy=off;found=1;}
  if(!next)break;
  if(k==255||off>UINT32_MAX-next*4)return 0;
  off+=next*4;
 }
 if(!found)return 1;
 if(!rd(i,legacy,&v)||(v&255)!=1)return 0;
 if(!(v&OS_OWNED)){if(!write8||!i->delay_ms||!write8(i->ctx,legacy+3,1))return 0;}
 if(!i->delay_ms)return 0;
 for(unsigned k=0;k<=1000;k++){
  if(!rd(i,legacy,&v)||(v&255)!=1||!(v&OS_OWNED))return 0;
  if(!(v&BIOS_OWNED)){uint32_t ctl;if(!rd(i,legacy+4,&ctl)||(ctl&LEGACY_SMI_ENABLE))return 0;return 1;}
  if(k<1000)i->delay_ms(i->ctx);
 }
 return 0;
}
