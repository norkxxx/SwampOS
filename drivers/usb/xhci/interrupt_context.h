#pragma once
#include "device.h"
#include "../hid/boot_hid.h"
/* Caller owns and initializes non-overlapping coherent transfer rings before
 * configuration. One directly attached, already configured boot HID device. */
struct xhci_interrupt_spec {unsigned dci,interval,packet,protocol;uint64_t ring_dma;};
int xhci_interrupt_spec(const struct usb_boot_interface*,unsigned,uint64_t,struct xhci_interrupt_spec*);
int xhci_interrupt_context(struct xhci_device_dma*,unsigned,unsigned,const struct xhci_interrupt_spec*,unsigned);
/* No transfers may be in flight. DMA remains pinned until Disable Slot or halt. */
int xhci_configure_interrupts(struct xhci_device*,const struct usb_boot_config*,const uint64_t*,struct xhci_interrupt_spec*);
