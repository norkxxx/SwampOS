# THE SWAMP AWAKENS — Build 22

**SwampOS target 0.20.0 / internal development checkpoint**

> ITS YOUR SWAMP - ITS YOUR LIFE

## USB2 Root Port Reset

Explicit root-port protocol discovery via xHCI extended capabilities, strict USB2 standard-speed profile, bounded power-on/debounce and port reset, preserving unrelated PORTSC change bits. No hardware backend is enabled.

## Validation

10,000 USB2 reset cycles, power/debounce, W1C preservation, USB3/PSI rejection, disconnect/timeout/overcurrent (UBSan). Full `make -j4 test`: exit 0. All 22 checksum entries verified. Hybrid ISO rebuilt. Protected SwampBoot artifacts unchanged. QEMU/VirtualBox unavailable; no guest or physical-hardware validation. No second clean-build reproducibility check.

Not a final release.
