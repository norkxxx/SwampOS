#include "device.h"
static int packet(unsigned speed,unsigned mps){return (speed==1&&(mps==8||mps==16||mps==32||mps==64))||(speed==2&&mps==8)||(speed==3&&mps==64);}
int xhci_address_context(struct xhci_device_dma*m,unsigned stride,unsigned port,unsigned speed,unsigned mps,uint64_t ep){if(!m||((uintptr_t)m&63)||(stride!=32&&stride!=64)||!port||port>255||!packet(speed,mps)||!ep||(ep&63))return 0;for(unsigned i=0;i<33*16;i++)m->input[i]=0;for(unsigned i=0;i<32*16;i++)m->output[i]=0;unsigned n=stride/4;uint32_t*slot=m->input+n,*ctx=m->input+2*n;m->input[1]=3;slot[0]=(speed<<20)|(1u<<27);slot[1]=port<<16;ctx[1]=(3u<<1)|(4u<<3)|(mps<<16);ctx[2]=(uint32_t)ep|1;ctx[3]=(uint32_t)(ep>>32);ctx[4]=8;return 1;}
static int overlap(uint64_t a,size_t an,uint64_t b,size_t bn){return a<b+bn&&b<a+an;}
static void fail(struct xhci_controller*c){c->state=1;(void)xhci_controller_stop(c);}
int xhci_device_address(struct xhci_device*d,struct xhci_controller*c,struct xhci_device_dma*m,uint64_t dma,struct xhci_ep0_dma*ep,uint64_t edma,unsigned port,unsigned speed,unsigned mps){if(!d||!c||c->state!=2||xhci_slots_full(c)||!c->memory||!m||!ep||!dma||(dma&65535)||dma>UINT64_MAX-sizeof(*m)||!edma||edma>UINT64_MAX-sizeof(*ep)||!port||port>c->caps.ports||!packet(speed,mps)||(!c->caps.ac64&&dma+sizeof(*m)>0x100000000ull)||overlap(dma,sizeof(*m),c->cmd.dma,sizeof(struct xhci_dma))||overlap(dma,sizeof(*m),edma,sizeof(*ep))||!xhci_address_context(m,c->caps.context_stride,port,speed,mps,edma))return 0;struct xhci_device v={.controller=c,.memory=m,.speed=speed,.ep0_packet=mps,.dma=dma};if(!xhci_ep0_init(&v.ep0,c,ep,edma))return 0;*d=v;if(!xhci_controller_command(c,9,0,0))return 0;d->ep0.ring.slot=c->last_slot;c->memory->dcbaa[d->ep0.ring.slot]=dma+offsetof(struct xhci_device_dma,output);__atomic_thread_fence(__ATOMIC_RELEASE);if(!xhci_controller_command(c,11,d->ep0.ring.slot,dma))return 0;__atomic_thread_fence(__ATOMIC_ACQUIRE);const volatile uint32_t*slot=m->output;if(((slot[3]>>27)&31)!=2||!(slot[3]&255)){fail(c);return 0;}d->addressed=1;return 1;}
int xhci_device_disable(struct xhci_device*d){if(!d||!d->addressed||!d->controller||d->ep0.ring.state||d->ep0.unusable)return 0;struct xhci_controller*c=d->controller;if(c->state!=2||xhci_slot_state(c,d->ep0.ring.slot)!=2||!xhci_controller_command(c,10,d->ep0.ring.slot,0))return 0;c->memory->dcbaa[d->ep0.ring.slot]=0;__atomic_thread_fence(__ATOMIC_RELEASE);d->addressed=0;d->configured=0;d->ep0.unusable=1;return 1;}

int xhci_device_probe_ep0(struct xhci_device*d){
 if(!d||!d->addressed||d->configured||!d->controller||!d->memory||d->ep0.ring.state||d->ep0.unusable)return 0;
 struct xhci_controller*c=d->controller;if(c->state!=2||xhci_slot_state(c,d->ep0.ring.slot)!=2||c->cmd.count||!d->dma||c->memory->dcbaa[d->ep0.ring.slot]!=d->dma+offsetof(struct xhci_device_dma,output))return 0;
 static const uint8_t setup[8]={0x80,6,0,1,0,0,8,0};uint8_t header[8];size_t actual;
 if(!xhci_ep0_transfer(&d->ep0,setup,header,8,&actual))return 0;
 if(actual!=8)goto fail;
 unsigned mps=header[7];if(header[0]!=18||header[1]!=1||!packet(d->speed,mps))goto fail;
 if(mps==d->ep0_packet)return 1;
 if(d->speed!=1)goto fail;
 unsigned n=c->caps.context_stride/4;const volatile uint32_t*old=d->memory->output+n;
 if((old[0]&7)!=1||(old[1]>>16)!=d->ep0_packet)goto fail;
 /* Evaluate Context updates only Max Packet Size for EP0. Preserve its
  * controller-owned dequeue and other endpoint state; no ring reinitialization. */
 for(unsigned i=0;i<33*16;i++)d->memory->input[i]=0;
 d->memory->input[1]=2;d->memory->input[2*n+1]=mps<<16;
 __atomic_thread_fence(__ATOMIC_RELEASE);if(!xhci_controller_command(c,13,d->ep0.ring.slot,d->dma))return 0;
 __atomic_thread_fence(__ATOMIC_ACQUIRE);if((old[0]&7)!=1||(old[1]>>16)!=mps)goto fail;
 d->ep0_packet=mps;return 1;
 fail:c->state=1;d->ep0.unusable=1;(void)xhci_controller_stop(c);return 0;
}
