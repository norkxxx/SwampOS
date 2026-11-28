# THE SWAMP AWAKENS — Build 28

**SwampOS target 0.20.0 / internal development checkpoint**

> ITS YOUR SWAMP - ITS YOUR LIFE

## Pinned DMA Memory

Kernel PMM/VM contiguous physical page leases with bounded alignment, generation tokens, pinning against ordinary page release, zeroing and explicit quiescence gate. No device activation or IOMMU setup.

## Validation

10,000 DMA lease cycles, stale-token rejection, pinning, halt gate, fragmentation, table exhaustion and supervisor-only mapping (UBSan). Full `make -j4 test`: exit 0. All 22 checksum entries verified. Hybrid ISO rebuilt. Protected SwampBoot artifacts unchanged. QEMU/VirtualBox unavailable; no guest or physical-hardware validation. No second clean-build reproducibility check.

Not a final release.
