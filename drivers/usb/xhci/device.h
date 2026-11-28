#pragma once
#include "control_transport.h"
/* Direct root-port devices only. No hubs, streams, scratchpads or SuperSpeed. */
struct xhci_device_dma {_Alignas(64) uint32_t input[33*16];_Alignas(64) uint32_t output[32*16];};
struct xhci_device {struct xhci_controller *controller;struct xhci_device_dma *memory;struct xhci_ep0 ep0;unsigned addressed,configured,speed,ep0_packet;uint64_t dma;};
int xhci_address_context(struct xhci_device_dma*,unsigned,unsigned,unsigned,unsigned,uint64_t);
/* Port must already be enabled after reset; speed 1=FS, 2=LS, 3=HS.
 * Caller owns coherent DMA areas; no allocations or physical controller binding. */
int xhci_device_address(struct xhci_device*,struct xhci_controller*,struct xhci_device_dma*,uint64_t,struct xhci_ep0_dma*,uint64_t,unsigned,unsigned,unsigned);
int xhci_device_disable(struct xhci_device*);

/* Full-speed EP0 discovery before USB enumeration / Configure Endpoint. */
int xhci_device_probe_ep0(struct xhci_device*);
