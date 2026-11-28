# THE SWAMP AWAKENS — Build 27

**SwampOS target 0.20.0 / internal development checkpoint**

> ITS YOUR SWAMP - ITS YOUR LIFE

## USB2 HID Boot Session

End-to-end single-interface USB2 boot-HID session: bounded DMA preflight, explicit root-port reset, Address Device, EP0 packet probe, USB enumeration, interrupt endpoint configuration and nonblocking input. All MMIO and DMA accesses are through a model in tests; no physical backend is activated.

## Validation

1,000 full modeled sessions and 20,000 keyboard reports, full/high speed, 32/64-byte contexts, invalid DMA/port rejection, stop and quarantine (UBSan). Full `make -j4 test`: exit 0. All 22 checksum entries verified. Hybrid ISO rebuilt. Protected SwampBoot artifacts unchanged. QEMU/VirtualBox unavailable; no guest or physical-hardware validation. No second clean-build reproducibility check.

Not a final release.
