# THE SWAMP AWAKENS — Build 19

**SwampOS target 0.20.0 / internal development checkpoint**

> ITS YOUR SWAMP - ITS YOUR LIFE

## xHCI EP0 Control TRBs

Single in-flight endpoint-zero Setup/Data/Status transfer descriptor ring, staged publication, DMA bounds, short IN tracking and transfer-event validation. Endpoint context and slot creation remain unimplemented.

## Validation

60,000 control transactions, setup/data/status, ring wrap, short IN packets, DMA bounds and poisoned endpoints (UBSan). Full `make -j4 test`: exit 0. All 22 checksum entries verified. Hybrid ISO rebuilt. Protected SwampBoot artifacts unchanged. QEMU/VirtualBox unavailable; no guest or physical-hardware validation. No second clean-build reproducibility check.

Not a final release.
