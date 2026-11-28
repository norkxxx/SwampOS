#pragma once
#include "controller.h"
/* Narrow direct USB 2.0 protocol profile: standard speed IDs, slot type 0.
 * No automatic power-on or reset in the Live system. Controller ownership
 * (including firmware handoff) and MMIO mapping are preconditions. */
int xhci_usb2_port(const struct xhci_controller*,unsigned);
/* Reset an explicitly selected root port; returns standard speed ID or zero. */
unsigned xhci_port_reset(struct xhci_controller*,unsigned);
