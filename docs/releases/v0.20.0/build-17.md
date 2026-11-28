# THE SWAMP AWAKENS — Build 17

**SwampOS target 0.20.0 / internal development checkpoint**

> ITS YOUR SWAMP - ITS YOUR LIFE

## xHCI Controller Lifecycle

Bounded callback-based MMIO controller lifecycle: capability validation, stop/reset, 4 KiB page support check, DCBAA/CRCR/ERST/ERDP programming, polling-mode start, No-op doorbell and completion, and bounded stop. Only a mock backend is provided; real PCI ownership, MMIO mapping and DMA are not enabled.

## Validation

10,000 modeled doorbell/completion exchanges, reset/run/stop, timeout, invalid completion, DMA quarantine and unsupported capability rejection (UBSan). Full `make -j4 test`: exit 0. All 22 checksum entries verified. Hybrid ISO rebuilt. Protected SwampBoot artifacts unchanged. QEMU/VirtualBox unavailable; no guest or physical-hardware validation. No second clean-build reproducibility check.

Not a final release.
