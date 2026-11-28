<div align="center">
<img src="assets/banner.svg" alt="Farkuad" width="100%" />

# SwampOS v0.13.1 — Farkuad | Take Control

**ITS YOUR SWAMP - ITS YOUR LIFE**

[![LIVE](https://img.shields.io/badge/LIVE-HYBRID_ISO-315c43?style=for-the-badge)](docs/LIVE.md)
[![Input](https://img.shields.io/badge/INPUT-PS2-39714c?style=for-the-badge)](docs/INPUT.md)
</div>

The Farkuad Live preview receives a native UEFI boot menu, a ten-second default timeout, explicit PS/2 controller initialization, optional COM1 detection, and relative PS/2 mouse support. A user confirmed boot of the previous ISO in VirtualBox, but reported missing input. The new code addresses candidate causes; guest execution of this patch is still pending.

- Four real boot choices: Live Desktop, keyboard-only desktop, COM1 recovery, return to firmware.
- ASCII ogre and the requested motto.
- PS/2 cursor and left-click desktop/file navigation.
- The 81 MiB hybrid ISO remains the primary distribution artifact.

Build: `bash scripts/build-live.sh`. Test: `make input-host-test`. See [test report](TEST-REPORT.md) and [input guide](docs/INPUT.md).

Experimental pre-release. No USB HID, absolute tablet, production disk encryption, or Secure Boot support in the Live image. Guest/hardware validation of this patch remains pending.
