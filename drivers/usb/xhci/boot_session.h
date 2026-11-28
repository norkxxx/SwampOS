#pragma once
#include "hid_transport.h"
#include "port.h"
#include "../core/enumeration.h"
/* One or two direct USB2 devices with one boot-HID interface each. All DMA ranges must
 * be coherent and pinned, PCI/firmware ownership already acquired, no IOMMU
 * translation changes. No PCI bus mastering or mapping performed here. */
struct xhci_boot_buffers {struct xhci_dma*c;struct xhci_device_dma*d;struct xhci_ep0_dma*ep;struct xhci_hid_dma*h;uint64_t cd,dd,ed,hd;};
struct xhci_boot_session {struct xhci_controller controller;struct xhci_device device;struct xhci_hid hid;struct usb_enumerated usb;struct xhci_device second_device;struct xhci_hid second_hid;struct usb_enumerated second_usb;uint8_t scratch[4096];unsigned state,count;};
/* states: 0 untouched, 1 active, 2 halted, 3 DMA possibly active (quarantine).
 * Zero initialize before start; no implicit restart. Only state 0/2 permits
 * caller to reclaim DMA, after separately revoking platform ownership. */
int xhci_boot_start(struct xhci_boot_session*,const struct xhci_io*,const struct xhci_boot_buffers*,unsigned);
int xhci_boot_poll(struct xhci_boot_session*,struct xhci_hid_event*);
int xhci_boot_stop(struct xhci_boot_session*);

int xhci_boot_start_ports(struct xhci_boot_session*,const struct xhci_io*,const struct xhci_boot_buffers*,const unsigned*,unsigned);
