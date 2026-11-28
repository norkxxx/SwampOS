# THE SWAMP AWAKENS — Build 30

**SwampOS target 0.20.0 / internal development checkpoint**

> ITS YOUR SWAMP - ITS YOUR LIFE

## Explicit Firmware Ownership

Bounded BIOS/OS legacy semaphore handshake using byte-wide OS ownership writes, extended capability chain validation, firmware SMI enable rejection and timeout without forced takeover. Not connected to physical PCI or DMA activation.

## Validation

5,000 cooperative firmware handoffs, timeout, malformed chains, SMI gate and byte-only ownership writes (UBSan). Full `make -j4 test`: exit 0. All 22 checksum entries verified. Hybrid ISO rebuilt. Protected SwampBoot artifacts unchanged. QEMU/VirtualBox unavailable; no guest or physical-hardware validation. No second clean-build reproducibility check.

Not a final release.
