#pragma once
#include "ring.h"
/* Single in-flight EP0 transaction; caller owns an enabled, addressed slot
 * and a coherent DMA ring. No PCI/MMIO or endpoint-context programming here. */
struct xhci_control_ring {volatile struct xhci_trb *trbs;uint64_t dma,setup_ptr,data_ptr,status_ptr;unsigned head,cycle,slot,state,length,actual,short_seen,in;};
/* state: 0 idle; 1 pending; 2 poisoned (stop/reset endpoint before reuse). */
int xhci_control_init(struct xhci_control_ring*,volatile struct xhci_trb*,uint64_t,unsigned);
int xhci_control_submit(struct xhci_control_ring*,const uint8_t[8],uint64_t,size_t);
/* 1 completed (actual returned), 0 short-data event / pending, -1 poisoned. */
int xhci_control_event(struct xhci_control_ring*,const struct xhci_trb*,unsigned*);
int xhci_control_selftest(void);
