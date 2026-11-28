#include "interrupt.h"
static void publish(volatile struct xhci_trb*t,uint64_t p,uint32_t status,uint32_t control){t->lo=(uint32_t)p;t->hi=(uint32_t)(p>>32);t->status=status;__atomic_thread_fence(__ATOMIC_RELEASE);t->control=control;}
int xhci_interrupt_init(struct xhci_interrupt_ring*r,volatile struct xhci_trb*p,const struct xhci_interrupt_spec*s){
 if(!r||!p||!s||((uintptr_t)p&63)||!s->ring_dma||(s->ring_dma&63)||s->ring_dma>UINT64_MAX-1024||(s->ring_dma&65535)>64512||s->dci<3||s->dci>31||!(s->dci&1)||!s->packet||s->packet>64)return 0;
 for(unsigned i=0;i<64;i++)p[i]=(struct xhci_trb){0};
 *r=(struct xhci_interrupt_ring){.trbs=p,.dma=s->ring_dma,.cycle=1,.slot=1,.dci=s->dci,.packet=s->packet};publish(p+63,r->dma,0,XHCI_TRB_TYPE(6)|2);return 1;
}
int xhci_interrupt_submit(struct xhci_interrupt_ring*r,uint64_t dma,size_t n){
 if(!r||!r->trbs||r->state||!dma||n!=r->packet||dma>UINT64_MAX-n||(dma&65535)+n>65536||(dma<r->dma+1024&&r->dma<dma+n))return 0;
 unsigned i=r->head,cycle=r->cycle;r->pending=r->dma+i*16;r->state=1;
 /* Make the link valid before publishing the last Normal TRB. */
 if(i==62){publish(r->trbs+63,r->dma,0,XHCI_TRB_TYPE(6)|2|cycle);r->head=0;r->cycle^=1;}else r->head++;
 publish(r->trbs+i,dma,(uint32_t)n,XHCI_TRB_TYPE(1)|(1u<<5)|(1u<<2)|cycle);return 1;
}
int xhci_interrupt_event(struct xhci_interrupt_ring*r,const struct xhci_trb*e,unsigned*actual){
 if(!r||!e||!actual)return -1;
 *actual=0;if(r->state==2)return -1;
 if(((e->control>>10)&63)!=32)return 0;
 uint64_t p=(uint64_t)e->hi<<32|e->lo;unsigned code=e->status>>24,residual=e->status&0xffffff;
 if(r->state!=1||(e->control>>24)!=r->slot||((e->control>>16)&31)!=r->dci||(e->control&4)||p!=r->pending||(code!=1&&code!=13)||residual>r->packet||(code==1&&residual)||(code==13&&!residual)){r->state=2;return -1;}
 __atomic_thread_fence(__ATOMIC_ACQUIRE);*actual=r->packet-residual;r->pending=0;r->state=0;return 1;
}
int xhci_interrupt_selftest(void){_Alignas(64) struct xhci_trb t[64];struct xhci_interrupt_ring r;struct xhci_interrupt_spec s={3,3,8,1,0x10000};unsigned n;if(!xhci_interrupt_init(&r,t,&s))return 0;for(unsigned i=0;i<256;i++){if(!xhci_interrupt_submit(&r,0x20000,8))return 0;struct xhci_trb e={(uint32_t)r.pending,0,1u<<24,XHCI_TRB_TYPE(32)|(1u<<24)|(3u<<16)};if(xhci_interrupt_event(&r,&e,&n)!=1||n!=8)return 0;}return !r.state;}
