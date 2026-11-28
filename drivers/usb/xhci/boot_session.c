#include "boot_session.h"
static int range(uint64_t a,size_t n,uint64_t align){return a&&!(a&(align-1))&&a<=UINT64_MAX-n;}
static int disjoint(const uint64_t*a,const size_t*n,unsigned count){for(unsigned i=0;i<count;i++)for(unsigned j=0;j<i;j++)if(a[i]<a[j]+n[j]&&a[j]<a[i]+n[i])return 0;return 1;}
static int buffers(const struct xhci_boot_buffers*b,const struct xhci_caps*c){if(!b)return 0;uint64_t phys[]={b->cd,b->dd,b->ed,b->hd},cpu[]={(uintptr_t)b->c,(uintptr_t)b->d,(uintptr_t)b->ep,(uintptr_t)b->h};size_t len[]={sizeof(*b->c),sizeof(*b->d),sizeof(*b->ep),sizeof(*b->h)};for(unsigned i=0;i<4;i++){if(!range(phys[i],len[i],i?65536:4096)||!range(cpu[i],len[i],64)||(!c->ac64&&phys[i]+len[i]>0x100000000ull))return 0;}return disjoint(phys,len,4)&&disjoint(cpu,len,4);}
int xhci_boot_stop(struct xhci_boot_session*s){if(!s||!s->state)return 0;if(s->state==2)return 1;if(s->controller.state==3){s->state=2;return 1;}s->state=3;if(s->hid.memory){if(!xhci_hid_abort(&s->hid))return 0;}else if(!xhci_controller_stop(&s->controller))return 0;s->state=2;return 1;}
static int attach(struct xhci_boot_session *s,const struct xhci_boot_buffers *b,unsigned port,unsigned index){
 struct xhci_device *d=index?&s->second_device:&s->device;
 struct xhci_hid *h=index?&s->second_hid:&s->hid;
 struct usb_enumerated *u=index?&s->second_usb:&s->usb;
 unsigned speed=xhci_port_reset(&s->controller,port);
 if(!speed||!xhci_device_address(d,&s->controller,b->d,b->dd,b->ep,b->ed,port,speed,speed==3?64:8)||
    !xhci_device_probe_ep0(d))return 0;
 if(usb_enumerate_boot(xhci_ep0_transfer,&d->ep0,s->scratch,sizeof s->scratch,u)!=1||
    u->ep0_size!=d->ep0_packet||u->config.count!=1)return 0;
 if(!xhci_hid_prepare(h,d,b->h,b->hd,u->config.interfaces))return 0;
 struct xhci_interrupt_spec spec[USB_BOOT_HID_MAX];
 return xhci_configure_interrupts(d,&u->config,&b->hd,spec);
}

static int separate_devices(const struct xhci_boot_buffers *a,const struct xhci_boot_buffers *b){
 if(a->c!=b->c||a->cd!=b->cd)return 0;
 uint64_t io[]={a->dd,a->ed,a->hd,b->dd,b->ed,b->hd};
 uint64_t cpu[]={(uintptr_t)a->d,(uintptr_t)a->ep,(uintptr_t)a->h,(uintptr_t)b->d,(uintptr_t)b->ep,(uintptr_t)b->h};
 size_t len[]={sizeof(*a->d),sizeof(*a->ep),sizeof(*a->h),sizeof(*b->d),sizeof(*b->ep),sizeof(*b->h)};
 return disjoint(io,len,6)&&disjoint(cpu,len,6);
}

int xhci_boot_start_ports(struct xhci_boot_session *s,const struct xhci_io *io,
                          const struct xhci_boot_buffers *b,const unsigned *ports,unsigned count){
 if(!s||s->state||s->controller.state||!io||!io->write32||!io->delay_ms||!b||!ports||!count||count>2)return 0;
 struct xhci_controller probe={.io=*io};
 if(!xhci_caps_read(io,&probe.caps)||probe.caps.scratchpads||count>probe.caps.slots)return 0;
 for(unsigned i=0;i<count;i++)
  if(!buffers(b+i,&probe.caps)||!xhci_usb2_port(&probe,ports[i]))return 0;
 if(count==2&&(ports[0]==ports[1]||!separate_devices(b,b+1)))return 0;
 s->state=3;s->count=count;
 if(!xhci_controller_start_slots(&s->controller,io,b->c,b->cd,count)){
  if(!s->controller.state)s->state=0;else (void)xhci_boot_stop(s);return 0;
 }
 for(unsigned i=0;i<count;i++)if(!attach(s,b+i,ports[i],i)){(void)xhci_boot_stop(s);return 0;}
 s->state=1;return 1;
}

int xhci_boot_start(struct xhci_boot_session *s,const struct xhci_io *io,const struct xhci_boot_buffers *b,unsigned port){
 return xhci_boot_start_ports(s,io,b,&port,1);
}

int xhci_boot_poll(struct xhci_boot_session *s,struct xhci_hid_event *out){
 if(!out)return -1;
 *out=(struct xhci_hid_event){0};
 if(!s||s->state!=1||!s->count||s->count>2)return -1;
 struct xhci_hid *inputs[]={&s->hid,&s->second_hid};
 for(unsigned i=0;i<s->count;i++)if(!inputs[i]->ring.state&&!xhci_hid_submit(inputs[i]))goto fail;
 int got=xhci_hid_poll_group(inputs,s->count,out);
 if(got>=0)return got;
fail:
 (void)xhci_boot_stop(s);return -1;
}
