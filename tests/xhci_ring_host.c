#include "../drivers/usb/xhci/ring.h"
#include <assert.h>
#include <stdio.h>
int main(void){_Alignas(64) struct xhci_trb c[64],e[64];struct xhci_cmd_ring cr;struct xhci_event_ring er;uint64_t p[62],n;struct xhci_trb out;
assert(xhci_ring_selftest());assert(!xhci_cmd_init(&cr,c,0xffc0));assert(!xhci_cmd_init(&cr,c,1));assert(xhci_cmd_init(&cr,c,0x10000));assert(xhci_event_init(&er,e,0x20000));assert(!xhci_event_pop(&er,&out,&n));
for(unsigned batch=0;batch<2000;batch++){for(unsigned i=0;i<62;i++){unsigned head=cr.head,cycle=cr.cycle;assert(xhci_cmd_noop(&cr,&p[i]));assert(p[i]==0x10000+head*16);assert(c[head].control==(XHCI_TRB_TYPE(23)|cycle));if(head==62)assert(c[63].control==(XHCI_TRB_TYPE(6)|2|cycle));}assert(!xhci_cmd_noop(&cr,&n));for(unsigned i=0;i<62;i++){struct xhci_trb bad={(uint32_t)(p[i]+1),0,1u<<24,XHCI_TRB_TYPE(33)};assert(xhci_cmd_complete(&cr,&bad)==-1);unsigned j=er.head;e[j]=(struct xhci_trb){(uint32_t)p[i],0,1u<<24,XHCI_TRB_TYPE(33)|er.cycle};assert(xhci_event_pop(&er,&out,&n)==1);assert(n==0x20000+er.head*16);assert(xhci_cmd_complete(&cr,&out)==1);}assert(!cr.count);assert(!xhci_event_pop(&er,&out,&n));}
assert(xhci_cmd_noop(&cr,&n));out=(struct xhci_trb){(uint32_t)n,0,5u<<24,XHCI_TRB_TYPE(33)};assert(xhci_cmd_complete(&cr,&out)==-1&&!cr.count);puts("XHCI_RING_HOST_OK: 124000 submissions/completions, wrap, full, invalid pointers, error completion");}
