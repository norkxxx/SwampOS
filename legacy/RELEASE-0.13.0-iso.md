<div align="center">
<img src="assets/banner.svg" alt="Farkuad" width="100%" />

# SwampOS v0.13.0 — Farkuad

**Security has layers. The swamp goes live.**

[![Live preview](https://img.shields.io/badge/LIVE-PREVIEW-315c43?style=for-the-badge)](docs/LIVE.md)
</div>

Farkuad is the first removable-media Live preview of SwampOS. It introduces a kernel-backed framebuffer desktop, five keyboard-driven utilities, an embedded read-only EXT4 volume, and a standalone x64 UEFI USB image. No Linux kernel, installation or second disk is needed in the intended boot path.

- **Live desktop:** terminal, notes, calculator, monitor and file previews.
- **Native applications:** existing ELF demos on the embedded RAM disk, with explicit runtime grants.
- **Boot:** separate unsigned removable-media loader, 81 MiB hybrid ISO9660 / El Torito + MBR/FAT32 image, no production vault unlock.
- **Security:** supervisor-only NX framebuffer, bounded RAM-disk reads, physical ATA mounting and NIC lab disabled in Live mode.
- **Release tooling:** build script, guarded Linux flash helper, checksums, updated Git LFS configuration and QEMU/OVMF USB runtime gate.

## ISO revision

The kernel version remains v0.13.0. The primary Live artifact is now a hybrid UEFI ISO, with optical and USB paths, both statically verified. No additional desktop functionality is claimed by this packaging revision.

## Download and verify

The complete ZIP includes `build/Farkuad-Live.iso`, its SHA-256, source, documentation, assets, test report and all previous lab images. Flash only the Farkuad ISO in raw/DD mode to a disposable USB drive. See [Live guide](docs/LIVE.md).

## Release classification

Experimental pre-release / first Live preview. Host testing and static image verification are not guest or hardware validation. QEMU/OVMF and real hardware remain unverified. No native USB HID or mouse, secure installed disk encryption, production sandbox, full window manager, actual VPN or DEB/RPM/EXE execution.

**SWAMP MATRIX // SWAMP OS**  
`YOUR SYSTEM. YOUR RULES. YOUR SWAMP.`
