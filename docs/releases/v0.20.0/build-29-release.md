# THE SWAMP AWAKENS — Build 29

**SwampOS target 0.20.0 / internal development checkpoint**

> ITS YOUR SWAMP - ITS YOUR LIFE

## PCI BAR and UC MMIO Mapping

Read-only PCI xHCI BAR validation against an explicitly supplied trusted resource, supervisor-only NX UC mapping in an isolated kernel PML4 branch, framebuffer alias rejection and bounded register callbacks. No BAR probing writes, controller reset, bus mastering or DMA activation.

## Validation

PCI resource validation and 10,000 bounded MMIO operations, UC/NX/supervisor PTE checks, physical alias and OOM rollback (UBSan). Full `make -j4 test`: exit 0. All 22 checksum entries verified. Hybrid ISO rebuilt. Protected SwampBoot artifacts unchanged. QEMU/VirtualBox unavailable; no guest or physical-hardware validation. No second clean-build reproducibility check.

Not a final release.
