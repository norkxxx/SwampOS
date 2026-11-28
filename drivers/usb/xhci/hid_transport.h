#pragma once
#include "interrupt.h"
/* One polled HID endpoint. Prepare before Configure Endpoint. Exclusive use of
 * controller event ring is required while an interrupt transfer is pending.
 * DMA memory must remain pinned until a confirmed halt/Disable Slot. */
struct xhci_hid_dma {_Alignas(64) struct xhci_trb ring[64];_Alignas(64) uint8_t report[64];};
struct xhci_hid {struct xhci_device*device;struct xhci_hid_dma*memory;struct xhci_interrupt_ring ring;struct hid_keyboard keyboard;unsigned protocol,failed;uint64_t dma;};
struct xhci_hid_event {unsigned protocol;struct hid_keys keys;struct hid_mouse mouse;};
int xhci_hid_prepare(struct xhci_hid*,struct xhci_device*,struct xhci_hid_dma*,uint64_t,const struct usb_boot_interface*);
int xhci_hid_submit(struct xhci_hid*);
/* Poll one event: 1 decoded input report, 0 pending or dropped invalid report,
 * -1 fatal; caller decides timeout and must call abort before abandoning DMA. */
int xhci_hid_poll(struct xhci_hid*,struct xhci_hid_event*);
int xhci_hid_abort(struct xhci_hid*);

int xhci_hid_poll_group(struct xhci_hid **,unsigned,struct xhci_hid_event *);
