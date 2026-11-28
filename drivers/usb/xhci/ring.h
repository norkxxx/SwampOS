#pragma once
#include <stdint.h>
#include <stddef.h>
#define XHCI_RING_COUNT 64u
#define XHCI_TRB_TYPE(t) ((uint32_t)(t)<<10)
struct xhci_trb {uint32_t lo,hi,status,control;};
struct xhci_cmd_ring {volatile struct xhci_trb *trbs;uint64_t dma;unsigned head,tail,count,cycle;};
struct xhci_event_ring {volatile struct xhci_trb *trbs;uint64_t dma;unsigned head,cycle;};
int xhci_cmd_init(struct xhci_cmd_ring*,volatile struct xhci_trb*,uint64_t);
int xhci_event_init(struct xhci_event_ring*,volatile struct xhci_trb*,uint64_t);
/* Narrow command allowlist: No-op, Enable Slot, Disable Slot, Address Device, Configure Endpoint, Evaluate Context. */
int xhci_cmd_submit(struct xhci_cmd_ring*,unsigned,unsigned,uint64_t,uint64_t*);
int xhci_cmd_noop(struct xhci_cmd_ring*,uint64_t*);
/* 1 completed, 0 not a completion, -1 unexpected pointer or completion code. */
int xhci_cmd_complete(struct xhci_cmd_ring*,const struct xhci_trb*);
int xhci_event_pop(struct xhci_event_ring*,struct xhci_trb*,uint64_t*);
int xhci_ring_selftest(void);
