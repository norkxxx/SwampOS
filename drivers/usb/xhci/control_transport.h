#pragma once
#include "controller.h"
#include "control.h"
/* DMA buffer remains owned by caller and pinned until controller halt has
 * been confirmed. No addressing/configuring endpoint contexts in this layer. */
struct xhci_ep0_dma {_Alignas(64) struct xhci_trb ring[64];_Alignas(64) uint8_t bounce[4096];};
struct xhci_ep0 {struct xhci_controller *controller;struct xhci_ep0_dma *memory;struct xhci_control_ring ring;uint64_t bounce_dma;unsigned unusable;};
/* Prepare an idle ring before Address Device, or attach to a quiescent slot 1 and EP0
 * configured to this ring. Do not submit until Address Device succeeds. Caller serializes all command/event operations. */
int xhci_ep0_init(struct xhci_ep0*,struct xhci_controller*,struct xhci_ep0_dma*,uint64_t);
/* Matches usb_control_fn. Exact data count is returned, including short IN. */
int xhci_ep0_transfer(void*,const uint8_t[8],uint8_t*,size_t,size_t*);
