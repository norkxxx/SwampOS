#pragma once
#include "ring.h"
/* Caller must own the controller and provide a bounded UC MMIO aperture,
 * coherent DMA mappings and elapsed-time delay. No PCI or physical mapping here. */
struct xhci_io {void *ctx;int (*read32)(void*,uint32_t,uint32_t*);int (*write32)(void*,uint32_t,uint32_t);void (*delay_ms)(void*);uint32_t aperture;};
struct xhci_caps {uint32_t op,rt,db,ext;unsigned slots,ports,scratchpads,ac64,context_stride,ppc;};
struct xhci_dma {
 _Alignas(64) struct xhci_trb cmd[64];
 _Alignas(64) struct xhci_trb event[64];
 _Alignas(64) uint64_t dcbaa[256];
 _Alignas(64) uint32_t erst[4];
};
struct xhci_controller {struct xhci_io io;struct xhci_caps caps;struct xhci_cmd_ring cmd;struct xhci_event_ring event;struct xhci_dma *memory;unsigned state,slot_state,second_slot_state,slot_limit,last_slot;};
/* states: 0 uninitialized; 1 DMA may be active, keep buffers pinned; 2 running; 3 confirmed halted */
int xhci_caps_read(const struct xhci_io*,struct xhci_caps*);
static inline unsigned xhci_slot_state(const struct xhci_controller *c,unsigned slot){
 return slot==1?c->slot_state:slot==2?c->second_slot_state:0;
}
static inline int xhci_slots_full(const struct xhci_controller *c){
 return c->slot_state&&(c->slot_limit<2||c->second_slot_state);
}
int xhci_controller_start_slots(struct xhci_controller*,const struct xhci_io*,struct xhci_dma*,uint64_t,unsigned);
int xhci_controller_start(struct xhci_controller*,const struct xhci_io*,struct xhci_dma*,uint64_t);
int xhci_controller_command(struct xhci_controller*,unsigned,unsigned,uint64_t);
int xhci_controller_noop(struct xhci_controller*);
int xhci_controller_stop(struct xhci_controller*);
