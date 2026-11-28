# THE SWAMP AWAKENS — Build 20

**SwampOS target 0.20.0 / internal development checkpoint**

> ITS YOUR SWAMP - ITS YOUR LIFE

## Integrated xHCI EP0 Transport

Control endpoint bridge joining USB enumeration, xHCI transfer and event rings, MMIO doorbell polling and bounded DMA bounce buffers. Model assumes an already-addressed slot and an EP0 context programmed to this ring. No hardware backend is enabled.

## Validation

Integrated enumeration of two HID interfaces, 10,000 read transactions, OUT and short IN, fault/timeout and DMA retention on failed halt (UBSan). Full `make -j4 test`: exit 0. All 22 checksum entries verified. Hybrid ISO rebuilt. Protected SwampBoot artifacts unchanged. QEMU/VirtualBox unavailable; no guest or physical-hardware validation. No second clean-build reproducibility check.

Not a final release.
