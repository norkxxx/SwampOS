#include "interrupt_context.h"
static int range(uint64_t a,size_t n){return a&&a<=UINT64_MAX-n;}
static int overlap(uint64_t a,size_t an,uint64_t b,size_t bn){return a<b+bn&&b<a+an;}
int xhci_interrupt_spec(const struct usb_boot_interface*i,unsigned speed,uint64_t dma,struct xhci_interrupt_spec*out){
 if(!i||!out||(speed<1||speed>3)||(i->endpoint&0xf0)!=0x80||!(i->endpoint&15)||i->protocol<1||i->protocol>2||i->packet_size<(i->protocol==1?8:3)||i->packet_size>(speed==2?8:64)||!i->interval||!range(dma,1024)||(dma&63)||(dma&65535)>64512)return 0;
 unsigned interval=0;if(speed==3){if(i->interval>16)return 0;interval=i->interval-1;}else{unsigned uf=(unsigned)i->interval*8;while(uf>1){uf>>=1;interval++;}}
 *out=(struct xhci_interrupt_spec){(unsigned)(i->endpoint&15)*2+1,interval,i->packet_size,i->protocol,dma};return 1;
}
int xhci_interrupt_context(struct xhci_device_dma*m,unsigned stride,unsigned speed,const struct xhci_interrupt_spec*s,unsigned count){
 if(!m||((uintptr_t)m&63)||(stride!=32&&stride!=64)||speed<1||speed>3||!s||!count||count>USB_BOOT_HID_MAX)return 0;
 unsigned n=stride/4,mask=1,high=1;const volatile uint32_t*slot=m->output;
 uint32_t slot0=slot[0],slot1=slot[1],slot2=slot[2];if(((slot0>>20)&15)!=speed||((slot0>>27)&31)!=1||((slot[3]>>27)&31)!=2||!(slot[3]&255))return 0;
 for(unsigned i=0;i<count;i++){const struct xhci_interrupt_spec*p=s+i;if(p->dci<3||p->dci>31||!(p->dci&1)||(mask&(1u<<p->dci))||p->protocol<1||p->protocol>2||p->packet<(p->protocol==1?8u:3u)||p->packet>(speed==2?8u:64u)||p->interval>15||(speed!=3&&(p->interval<3||p->interval>10))||!range(p->ring_dma,1024)||(p->ring_dma&63)||(p->ring_dma&65535)>64512)return 0;for(unsigned j=0;j<i;j++)if(overlap(p->ring_dma,1024,s[j].ring_dma,1024))return 0;mask|=1u<<p->dci;if(p->dci>high)high=p->dci;}
 for(unsigned i=0;i<33*16;i++)m->input[i]=0;
 m->input[1]=mask;m->input[n]=(slot0&0x07ffffffu)|(high<<27);m->input[n+1]=slot1;m->input[n+2]=slot2;
 for(unsigned i=0;i<count;i++){const struct xhci_interrupt_spec*p=s+i;uint32_t*ep=m->input+(p->dci+1)*n;ep[0]=p->interval<<16;ep[1]=(3u<<1)|(7u<<3)|(p->packet<<16);ep[2]=(uint32_t)p->ring_dma|1;ep[3]=(uint32_t)(p->ring_dma>>32);ep[4]=p->packet|(p->packet<<16);}
 return 1;
}
int xhci_configure_interrupts(struct xhci_device*d,const struct usb_boot_config*cfg,const uint64_t*ring,struct xhci_interrupt_spec*out){
 if(!d||!d->addressed||d->configured||!d->controller||!d->memory||!cfg||!cfg->configuration||!ring||!out||!cfg->count||cfg->count>USB_BOOT_HID_MAX||d->ep0.ring.state||d->ep0.unusable)return 0;
 struct xhci_controller*c=d->controller;struct xhci_interrupt_spec tmp[USB_BOOT_HID_MAX];if(c->state!=2||xhci_slot_state(c,d->ep0.ring.slot)!=2||c->cmd.count||!range(d->dma,sizeof(*d->memory))||!range(c->cmd.dma,sizeof(struct xhci_dma))||!range(d->ep0.ring.dma,sizeof(struct xhci_ep0_dma))||c->memory->dcbaa[d->ep0.ring.slot]!=d->dma+offsetof(struct xhci_device_dma,output))return 0;
 for(unsigned i=0;i<cfg->count;i++){uint64_t a=ring[i];if(!xhci_interrupt_spec(cfg->interfaces+i,d->speed,a,tmp+i)||(!c->caps.ac64&&a+1024>0x100000000ull)||overlap(a,1024,c->cmd.dma,sizeof(struct xhci_dma))||overlap(a,1024,d->dma,sizeof(*d->memory))||overlap(a,1024,d->ep0.ring.dma,sizeof(struct xhci_ep0_dma)))return 0;}
 if(!xhci_interrupt_context(d->memory,c->caps.context_stride,d->speed,tmp,cfg->count))return 0;
 __atomic_thread_fence(__ATOMIC_RELEASE);if(!xhci_controller_command(c,12,d->ep0.ring.slot,d->dma))return 0;
 __atomic_thread_fence(__ATOMIC_ACQUIRE);unsigned n=c->caps.context_stride/4;const volatile uint32_t*slot=d->memory->output;if(((slot[3]>>27)&31)!=3)goto fail;for(unsigned i=0;i<cfg->count;i++)if((slot[tmp[i].dci*n]&7)!=1)goto fail;
 for(unsigned i=0;i<cfg->count;i++){out[i]=tmp[i];}d->configured=1;return 1;
 fail:c->state=1;(void)xhci_controller_stop(c);return 0;
}
