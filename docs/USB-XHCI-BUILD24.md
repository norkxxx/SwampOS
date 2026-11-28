# xHCI Interrupt Transfer Ring

Single outstanding interrupt IN Normal TRB per endpoint, bounded report buffer, validated event identity and length, wrap and cycle transitions, fail-stop. No physical hardware backend.

40,000 modeled interrupt transfers, ring wrap, short IN, malformed events, ownership and bounds (UBSan).

`xhci-ring-test` runs the software ring test; it does not detect or drive a USB device. Controller transport is not connected to the PCI hardware backend, and no new Live disk writes or DMA activation are permitted.

Reference: [xHCI specification, Intel](https://cdrdv2-public.intel.com/625472/625472_xHCI_Rev1_2b.pdf).
