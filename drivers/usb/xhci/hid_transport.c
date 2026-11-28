#include "hid_transport.h"
static int wr(struct xhci_controller*c,uint32_t o,uint32_t v){return c->io.aperture>=4&&!(o&3)&&o<=c->io.aperture-4&&c->io.write32(c->io.ctx,o,v);}
static int rd(struct xhci_controller*c,uint32_t o,uint32_t*v){return c->io.aperture>=4&&!(o&3)&&o<=c->io.aperture-4&&c->io.read32(c->io.ctx,o,v);}
static int overlap(uint64_t a,size_t n,uint64_t b,size_t m){return !b||b>UINT64_MAX-m|| (a<b+m&&b<a+n);}
static void wipe(struct xhci_hid*h){volatile uint8_t*p=h->memory->report;for(unsigned i=0;i<64;i++)p[i]=0;}
int xhci_hid_prepare(struct xhci_hid*h,struct xhci_device*d,struct xhci_hid_dma*m,uint64_t dma,const struct usb_boot_interface*i){
 if(!h||!d||!d->addressed||d->configured||!d->controller||!m||((uintptr_t)m&63)||!dma||(dma&65535)||dma>UINT64_MAX-sizeof(*m))return 0;
 struct xhci_controller*c=d->controller;struct xhci_interrupt_spec spec;
 if(c->state!=2||xhci_slot_state(c,d->ep0.ring.slot)!=2||c->cmd.count||d->ep0.ring.state||d->ep0.unusable||(!c->caps.ac64&&dma+sizeof(*m)>0x100000000ull)||overlap(dma,sizeof(*m),c->cmd.dma,sizeof(struct xhci_dma))||overlap(dma,sizeof(*m),d->dma,sizeof(struct xhci_device_dma))||overlap(dma,sizeof(*m),d->ep0.ring.dma,sizeof(struct xhci_ep0_dma))||!xhci_interrupt_spec(i,d->speed,dma,&spec))return 0;
 struct xhci_hid v={.device=d,.memory=m,.protocol=spec.protocol,.dma=dma};if(!xhci_interrupt_init(&v.ring,m->ring,&spec))return 0;v.ring.slot=d->ep0.ring.slot;*h=v;wipe(h);return 1;
}
int xhci_hid_abort(struct xhci_hid*h){if(!h||!h->device||!h->memory||!h->device->controller)return 0;struct xhci_controller*c=h->device->controller;h->failed=1;h->ring.state=2;if(c->state!=3){c->state=1;if(!xhci_controller_stop(c))return 0;}wipe(h);h->keyboard=(struct hid_keyboard){0};return 1;}
int xhci_hid_submit(struct xhci_hid*h){if(!h||!h->device||!h->memory||h->failed||h->ring.state)return 0;struct xhci_device*d=h->device;struct xhci_controller*c=d->controller;
 if(!c||!d->addressed||!d->configured||c->state!=2||xhci_slot_state(c,d->ep0.ring.slot)!=2||c->cmd.count||d->ep0.ring.state||d->ep0.unusable||c->io.aperture<8||c->caps.db>c->io.aperture-8||!(c->caps.db+8<=c->caps.rt||c->caps.db>=c->caps.rt+0x40))return 0;
 /* The input context is retained by this narrow single-configuration driver.
  * Output dequeue can advance, so compare against the original configured ring. */
 unsigned n=c->caps.context_stride/4;const uint32_t*ep=d->memory->input+(h->ring.dci+1)*n;
 if(!(d->memory->input[1]&(1u<<h->ring.dci))||ep[2]!=((uint32_t)h->dma|1)||ep[3]!=(uint32_t)(h->dma>>32)||ep[1]>>16!=h->ring.packet)return 0;
 wipe(h);if(!xhci_interrupt_submit(&h->ring,h->dma+offsetof(struct xhci_hid_dma,report),h->ring.packet))return 0;
 if(!wr(c,c->caps.db+4*h->ring.slot,h->ring.dci)){(void)xhci_hid_abort(h);return 0;}return 1;
}
int xhci_hid_poll_group(struct xhci_hid **inputs,unsigned count,struct xhci_hid_event *out){
 if(!out||!inputs||!count||count>2)return -1;
 *out=(struct xhci_hid_event){0};
 struct xhci_controller *c=0;
 for(unsigned i=0;i<count;i++){
  struct xhci_hid *h=inputs[i];
  if(!h||!h->device||!h->memory||h->failed||h->ring.state!=1)return -1;
  if(i&&c!=h->device->controller)return -1;
  c=h->device->controller;
 }
 uint32_t status;
 if(!c||c->state!=2||!rd(c,c->caps.op+4,&status)||(status&((1u<<0)|(1u<<2)|(1u<<12))))goto fail;
 struct xhci_trb event;uint64_t next;
 int got=xhci_event_pop(&c->event,&event,&next);
 if(got<0)goto fail;
 if(!got)return 0;
 next|=8;
 if(!wr(c,c->caps.rt+0x38,(uint32_t)next)||!wr(c,c->caps.rt+0x3c,(uint32_t)(next>>32)))goto fail;
 if(((event.control>>10)&63)!=32)goto fail;
 for(unsigned i=0;i<count;i++){
  struct xhci_hid *h=inputs[i];
  if((event.control>>24)!=h->ring.slot||((event.control>>16)&31)!=h->ring.dci)continue;
  unsigned actual;
  if(xhci_interrupt_event(&h->ring,&event,&actual)!=1)goto fail;
  /* Boot mice may append wheel/vendor data after the three-byte prefix. */
  int valid=h->protocol==1?hid_keyboard_report(&h->keyboard,h->memory->report,actual,&out->keys):
   hid_mouse_report(h->memory->report,actual>3?3:actual,&out->mouse);
  wipe(h);if(!valid)return 0;
  out->protocol=h->protocol;return 1;
 }
fail:
 (void)xhci_hid_abort(inputs[0]);return -1;
}
int xhci_hid_poll(struct xhci_hid *h,struct xhci_hid_event *out){
 return xhci_hid_poll_group(&h,1,out);
}
