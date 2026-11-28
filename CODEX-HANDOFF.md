# SwampOS engineering handoff

## Current checkpoint · 0.20.0-dev.38-r1

The AHCI and NVMe profiles mount one fixed file, `/persist/note`, through the block/VFS layer. Both transports survived an abrupt QEMU shutdown and recovered the preceding revision after on-disk corruption. The cumulative ZIP retains prior build contents. Read docs/PERSISTENCE.md, TEST-REPORT.md and the guest logs.

Next milestone is Build 39, SwampGuard and user-process permissions. The storage fixture is suitable for CRC-checked user text. Persisting trusted policy requires identity, authentication, controlled updates and rollback handling. Multiple DMA domains, dynamic files and writable EXT4 remain open.

## Build 37 handoff (historical)

Build 37 boots the cumulative Live desktop and adds native xHCI keyboard/mouse input in an opt-in Q35/OVMF profile. The kernel owns two HID slots and a restricted, legacy VT-d domain. The QEMU test disables i8042, types `version` on USB, drags and resizes Notes by USB mouse, then inspects the DMA tables independently. See docs/USB-INPUT.md and TEST-REPORT.md. The ISO hash is `d480878ebf9de1b3400407a4bb15d7a3142412d984fd0bfffebeee6e1942594a`.

The next planned subsystem is storage. Start with controller discovery and the existing block/VFS contracts, then exercise AHCI/NVMe on disposable disk images. Keep the Live root in RAM and preserve the disk-write boundary. Remaining USB work includes hubs, physical platform ownership and hardware qualification. The VirtualBox flicker retest remains open.

## Build 36 handoff (historical)

The integrated desktop renderer is in `desktop/compositor.c`. The production callers are in `kernel/desktop.c`. The clean build, aggregate host suite and six QEMU/OVMF routes are recorded in TEST-REPORT.md and build/logs. The Live ISO SHA-256 is `526cff3308caf630d832c6117f627a2e5085c2dea43fa1ee4031300829523d45`.

The next planned checkpoint is xHCI USB input with verified device isolation. Start from the Q35 `live-iommu-runtime` fixture. It exercises discovery, BAR validation, capability reads and DMAR scope matching. The guest input path in this checkpoint uses PS/2. Per-device DMA remapping, active kernel-owned xHCI and physical USB HID still require implementation and tests. The VirtualBox checklist for the reported flicker is in docs/VM-ACCEPTANCE.md.

---

# SWAMPOS — migration to Codex / Build 35

This transfer is a technical handoff, not a verbatim transcript. It packages the original Build 35 tree unchanged plus these migration documents. No new OS build, code modification or runtime test was performed during migration.

## Start here
1. Open the extracted `SwampOS-Farkuad` directory as the repository root (or merge the files into an existing checkout after comparing diffs; preserve `.git`). Do not overwrite newer commits blindly.
2. Read AGENTS.md and the existing checkpoint ledgers. Verify source and artifact hashes against the supplied archive. Record baseline `git status`, clean build and available test tools.
3. Begin Build 36: fix mouse-driven flicker and integrate double buffering, damage tracking, cursor rendering, bounded frame pacing and event coalescing while preserving Classic mode and current application behavior. See KNOWN-BUGS.md for reproduction and acceptance.
4. Continue by subsystem using ROADMAP.md, no fixed number of builds. Keep the current target version 0.20.0 until release acceptance. Update handoff and logs after each meaningful checkpoint.

## Verified archive facts at migration
- Original Build 35 ZIP: 652 entries, CRC check passed; 166 entries under logs; embedded `build/Farkuad-Live.iso` is 44,959,744 bytes. This is archive verification, not new guest validation.
- Build 35 TEST-REPORT records successful `make -j2 all`, `make acpi-dmar-host-test` (UBSan), `make -j4 live-host-test`, `make -j4 boot-host-test`. Full aggregate `make test` was not completed. QEMU/OVMF and VirtualBox were unavailable to the prior build environment. User confirmed Farkuad 0.13.1 in VirtualBox; user reports current mouse-driven flicker. Do not assume Build 35 guest tests otherwise passed.
- Independent C/ASM x86_64 kernel; hybrid UEFI ISO, embedded 8 MiB read-only EXT4, PS/2, window manager and ten kernel-trusted tools; RAMFS 64 files × 8 KiB and 64 directories; bounded experimental Ring 3 ELF and one resumable GUI client; resource snapshots up to 64 KiB, SwampGuard 32 volatile UUID+inode profiles and staged editor. See FEATURES.md for exact restrictions. Native apps are not a general isolated desktop ecosystem yet.
- USB xHCI rings, slots, EP0, HID transport, pinning and firmware ownership are model-tested. Builds 32–35 expose `xhci`, `xhci-fw`, `xhci-probe`, `iommu` for read-only diagnostics. Physical controller start, real DMA, active VT-d remapping and physical USB HID are NOT implemented. AMD IVRS and multihop DMAR scopes are not resolved.
- Dynamic ISO sizing introduced in Build 31. ISO size changes only when volume sizing thresholds are crossed. Build 31 initially lost historical logs; corrected archive restored them. Preserve them in every later checkpoint.
- Existing protected SwampBoot/SwampLogin route is separate from unsigned experimental Live boot. Full secure installed system, persistent storage, VPN, compatibility and CUDA are unfinished. Read MISSING.md.

## Commands (inspect existing Makefile before use)
```sh
make -j2 all
make -j4 live-host-test
make -j4 boot-host-test
make test
make checksums
make live-runtime
make live-optical-runtime
```
README documents Linux/Ubuntu WSL2, `./install.sh` and `bash scripts/build-live.sh`. Inspect installer before executing. VirtualBox user baseline: x64 EFI enabled, 1 CPU, 512–1024 MiB RAM, PS/2 Mouse, VBoxSVGA, no 3D acceleration; network off for the current offline Live route. Use disposable VM/storage. Keep Git LFS and run `python3 scripts/check-git-index.py`, `git lfs fsck` before commit.

## User working preferences
Senior infrastructure/security engineer style; concise Russian reports, English project documents, actual code and tests. Preserve every prior source, document, log and release artifact requirement. Large builds per integrated subsystem; do not stop for routine confirmation. Do not invent progress or promise continued execution outside the actual task. End with concrete checkpoint status, known limitations and commit commands.
