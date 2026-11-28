# Nonblocking HID Transport

Nonblocking submit/poll bridge for one prepared and configured interrupt IN endpoint, MMIO doorbell, event ring, keyboard/mouse boot decoding, report wiping, controller stop and DMA quarantine. Exclusive controller event ownership is required; physical backend remains disabled.

20,000 modeled keyboard/mouse reports, malformed and short reports, pending idle polls, abort and DMA quarantine (UBSan).

`xhci-interrupt-test` runs the software ring test; it does not detect or drive a USB device. Controller transport is not connected to the PCI hardware backend, and no new Live disk writes or DMA activation are permitted.

Reference: [xHCI specification, Intel](https://cdrdv2-public.intel.com/625472/625472_xHCI_Rev1_2b.pdf).
