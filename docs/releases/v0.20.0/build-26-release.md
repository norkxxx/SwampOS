# THE SWAMP AWAKENS — Build 26

**SwampOS target 0.20.0 / internal development checkpoint**

> ITS YOUR SWAMP - ITS YOUR LIFE

## Full-Speed EP0 Probe

Initial eight-byte device descriptor read and Evaluate Context command to update Full Speed EP0 Max Packet Size without resetting its dequeue state. Strict validation of speed/packet combinations and controller result. No physical hardware backend.

## Validation

4,000 full-speed MPS probes, 8/16/32/64 bytes, both context sizes, invalid size and failed context update (UBSan). Full `make -j4 test`: exit 0. All 22 checksum entries verified. Hybrid ISO rebuilt. Protected SwampBoot artifacts unchanged. QEMU/VirtualBox unavailable; no guest or physical-hardware validation. No second clean-build reproducibility check.

Not a final release.
