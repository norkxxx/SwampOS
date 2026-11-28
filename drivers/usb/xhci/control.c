#include "control.h"
#define TYPE(t) XHCI_TRB_TYPE(t)
static uint32_t pack(const uint8_t*p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static void publish(volatile struct xhci_trb*p,struct xhci_trb t){p->lo=t.lo;p->hi=t.hi;p->status=t.status;__atomic_thread_fence(__ATOMIC_RELEASE);p->control=t.control;}
int xhci_control_init(struct xhci_control_ring*r,volatile struct xhci_trb*p,uint64_t d,unsigned slot){if(!r||!p||((uintptr_t)p&63)||!d||(d&63)||d>UINT64_MAX-1024||(d&65535)+1024>65536||!slot||slot>255)return 0;for(unsigned i=0;i<64;i++)p[i]=(struct xhci_trb){0};*r=(struct xhci_control_ring){.trbs=p,.dma=d,.cycle=1,.slot=slot};publish(&p[63],(struct xhci_trb){(uint32_t)d,(uint32_t)(d>>32),0,TYPE(6)|2});return 1;}
static uint64_t append(struct xhci_control_ring*r,struct xhci_trb t,unsigned hold){unsigned j=r->head,cycle=r->cycle;t.control|=cycle^hold;publish(&r->trbs[j],t);if(++r->head==63){publish(&r->trbs[63],(struct xhci_trb){(uint32_t)r->dma,(uint32_t)(r->dma>>32),0,TYPE(6)|2|cycle});r->head=0;r->cycle^=1;}return r->dma+j*16;}
int xhci_control_submit(struct xhci_control_ring*r,const uint8_t s[8],uint64_t d,size_t n){if(!r||!r->trbs||r->state||!s||n!=((unsigned)s[6]|(unsigned)s[7]<<8)||n>4096||(!n&&d)||(n&&(!d||d>UINT64_MAX-n||(d&65535)+n>65536||(d<r->dma+1024&&r->dma<d+n))))return 0;unsigned in=!!(s[0]&128),start=r->head,cycle=r->cycle;r->length=r->actual=(unsigned)n;r->short_seen=0;r->in=in;r->data_ptr=0;
 r->setup_ptr=append(r,(struct xhci_trb){pack(s),pack(s+4),8,TYPE(2)|(1u<<6)|(n?(in?3u:2u)<<16:0)},1);
 if(n)r->data_ptr=append(r,(struct xhci_trb){(uint32_t)d,(uint32_t)(d>>32),(uint32_t)n,TYPE(3)|(in?((1u<<16)|(1u<<2)):0)},0);
 r->status_ptr=append(r,(struct xhci_trb){0,0,0,TYPE(4)|(1u<<5)|((!n||!in)?1u<<16:0)},0);
 r->state=1;__atomic_thread_fence(__ATOMIC_RELEASE);r->trbs[start].control=(r->trbs[start].control&~1u)|cycle;return 1;}
int xhci_control_event(struct xhci_control_ring*r,const struct xhci_trb*e,unsigned*actual){if(!r||!e||!actual||r->state!=1)return -1;uint64_t p=(uint64_t)e->hi<<32|e->lo;unsigned code=e->status>>24,res=e->status&0xffffff;if(((e->control>>10)&63)!=32||(e->control&4)||((e->control>>16)&31)!=1||(e->control>>24)!=r->slot)goto fail;
 if(code==13&&r->in&&r->data_ptr&&p==r->data_ptr&&!r->short_seen&&res<=r->length){r->short_seen=1;r->actual=r->length-res;return 0;}
 if(code==1&&p==r->status_ptr&&!res){*actual=r->actual;r->state=0;return 1;}
 fail:r->state=2;return -1;}
int xhci_control_selftest(void){_Alignas(64) struct xhci_trb t[64];struct xhci_control_ring r;uint8_t s[8]={0x80,6,0,1,0,0,18,0};unsigned actual;if(!xhci_control_init(&r,t,0x10000,1)||!xhci_control_submit(&r,s,0x20000,18))return 0;struct xhci_trb e={(uint32_t)r.status_ptr,0,1u<<24,TYPE(32)|(1u<<16)|(1u<<24)};return xhci_control_event(&r,&e,&actual)==1&&actual==18;}
