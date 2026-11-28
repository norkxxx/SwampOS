#include "pci_resource.h"
int xhci_pci_resource(pci_read_fn read,void*ctx,unsigned bus,unsigned dev,unsigned fn,uint64_t trusted,uint32_t bytes,struct xhci_pci_resource*out){
 if(!read||!out||bus>255||dev>31||fn>7||!trusted||(trusted&4095)||trusted>=1ull<<52||!bytes||(bytes&4095)||bytes>2u*1024*1024||bytes>(1ull<<52)-trusted)return 0;
 uint32_t id=read(ctx,bus,dev,fn,0),cls=read(ctx,bus,dev,fn,8),hdr=read(ctx,bus,dev,fn,12),cmd=read(ctx,bus,dev,fn,4),lo=read(ctx,bus,dev,fn,16);
 if(!(id&65535)||(id&65535)==65535||(cls>>8)!=0x0c0330||((hdr>>16)&127)||cmd==UINT32_MAX||lo==UINT32_MAX||(lo&9)||((lo&6)!=0&&(lo&6)!=4))return 0;
 uint64_t base=lo&~15u;if((lo&6)==4)base|=(uint64_t)read(ctx,bus,dev,fn,20)<<32;
 if(base!=trusted)return 0;
 *out=(struct xhci_pci_resource){base,bytes,!!(cmd&2),!!(cmd&4)};return 1;
}
static int valid(const struct xhci_mmio_window*w,uint32_t off){return w&&w->base&&!((uintptr_t)w->base&3)&&w->bytes>=4&&!(w->bytes&3)&&!(off&3)&&off<=w->bytes-4;}
int xhci_mmio_read32(void*p,uint32_t off,uint32_t*v){struct xhci_mmio_window*w=p;if(!v||!valid(w,off))return 0;*v=w->base[off/4];__atomic_thread_fence(__ATOMIC_ACQUIRE);return 1;}
int xhci_mmio_write32(void*p,uint32_t off,uint32_t v){struct xhci_mmio_window*w=p;if(!valid(w,off)||w->writable!=1)return 0;__atomic_thread_fence(__ATOMIC_RELEASE);w->base[off/4]=v;__atomic_thread_fence(__ATOMIC_SEQ_CST);return 1;}

int xhci_mmio_write8(void*p,uint32_t off,uint8_t v){struct xhci_mmio_window*w=p;if(!w||!w->base||off>=w->bytes||w->writable!=1)return 0;volatile uint8_t*b=(volatile uint8_t*)w->base;__atomic_thread_fence(__ATOMIC_RELEASE);b[off]=v;__atomic_thread_fence(__ATOMIC_SEQ_CST);return 1;}
