# xHCI Device Context & Slot Lifecycle

Single-slot Enable Slot, Address Device and Disable Slot commands; 32/64-byte input/output context layout, EP0 context, DCBAA ownership and address completion validation. Requires an enabled root port and caller-owned DMA areas. Hardware binding remains absent.

4,000 slot enable/address/disable lifecycles, 32/64-byte contexts, DMA overlap and invalid slot/output rejection (UBSan).

`xhci-control-test` runs the software ring test; it does not detect or drive a USB device. Controller transport is not connected to the PCI hardware backend, and no new Live disk writes or DMA activation are permitted.

Reference: [xHCI specification, Intel](https://cdrdv2-public.intel.com/625472/625472_xHCI_Rev1_2b.pdf).
