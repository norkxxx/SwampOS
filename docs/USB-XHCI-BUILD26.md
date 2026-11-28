# Full-Speed EP0 Probe

Initial eight-byte device descriptor read and Evaluate Context command to update Full Speed EP0 Max Packet Size without resetting its dequeue state. Strict validation of speed/packet combinations and controller result. No physical hardware backend.

4,000 full-speed MPS probes, 8/16/32/64 bytes, both context sizes, invalid size and failed context update (UBSan).

`xhci-interrupt-test` runs the software ring test; it does not detect or drive a USB device. Controller transport is not connected to the PCI hardware backend, and no new Live disk writes or DMA activation are permitted.

Reference: [xHCI specification, Intel](https://cdrdv2-public.intel.com/625472/625472_xHCI_Rev1_2b.pdf).
