# xHCI Interrupt Endpoint Contexts

Configure Endpoint command, USB2 interrupt-IN endpoint context generation, interval and packet validation, and DMA ring overlap rejection. Caller supplies initialized pinned coherent rings; physical backend remains disabled.

4,000 modeled configure/disable lifecycles, 32/64-byte contexts, DCI 31, interval, DMA overlap and failed endpoint state (UBSan).

`xhci-ring-test` runs the software ring test; it does not detect or drive a USB device. Controller transport is not connected to the PCI hardware backend, and no new Live disk writes or DMA activation are permitted.

Reference: [xHCI specification, Intel](https://cdrdv2-public.intel.com/625472/625472_xHCI_Rev1_2b.pdf).
