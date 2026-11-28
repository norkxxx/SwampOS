#pragma once
#include "interrupt_context.h"
/* One in-flight Normal TRB; caller guarantees exclusive ring/event ownership,
 * initialized endpoint context, coherent DMA, and no reuse before halt/disable. */
struct xhci_interrupt_ring {volatile struct xhci_trb*trbs;uint64_t dma,pending;unsigned head,cycle,slot,dci,packet,state;};
int xhci_interrupt_init(struct xhci_interrupt_ring*,volatile struct xhci_trb*,const struct xhci_interrupt_spec*);
int xhci_interrupt_submit(struct xhci_interrupt_ring*,uint64_t,size_t);
/* 1 completed; 0 no unrelated event consumed; -1 poisoned. Caller must
 * stop controller or disable endpoint before releasing memory on poison. */
int xhci_interrupt_event(struct xhci_interrupt_ring*,const struct xhci_trb*,unsigned*);
int xhci_interrupt_selftest(void);
