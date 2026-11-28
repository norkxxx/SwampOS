## Build 38 revision 1 · GitHub runner compatibility

- Target the baseline x86-64 ISA explicitly when building the freestanding kernel. This keeps newer Linux compiler defaults from emitting BMI2 instructions for the baseline guest CPU.
- Accept legacy VirtIO rings with up to 1,024 descriptors while keeping the receive buffer pool bounded at 256 packets.
- Run the kernel and GUI labs with a Secure Boot capable OVMF image and blank variables. The GUI lab reads QMP through private FIFOs.
- Preserve the prior Git BIOS image losslessly at `legacy/swampbox/SwampOS-from-git.img.gz`. Building regenerates the current BIOS lab image.
- Run the 14 AHCI/NVMe guest scenarios in GitHub Actions.

## Build 38 · Persistent storage

Added Q35 AHCI and NVMe drivers, firmware resource revalidation, bounded I/O under VT-d and the two-slot note volume. Added independent cold-boot, corrupt-sector and denial tests on both transports. The read-only Live root and protected boot chain are retained.

# Build 37 · USB input and DMA isolation

- Route native USB key and relative pointer reports through the Live shell and desktop.
- Manage two slots on a shared xHCI event ring. Dispatch interrupt completions by slot and endpoint.
- Bind the emulated xHCI requester to sparse VT-d mappings. Halt, disable mastering and invalidate mappings on session teardown.
- Add the interactive Q35 launcher and six guest verification profiles.

# Development target 0.20.0 — Build 36

- Render the scene offscreen in windowed and Classic modes. Track window geometry, stacking, focus, menu, content, theme and independent cursor damage.
- Batch presentation across input packets with a two-tick minimum interval. Restore pixels beneath moving cursors and displaced windows.
- Read the EDK II BAR alignment descriptor and decode the separate device/function bytes in ACPI DMAR paths.
- Exercise PS/2 cursor movement, drag, resize, keyboard, Classic, recovery, the locked loader and read-only xHCI/DMAR diagnostics in QEMU/OVMF.

# Development target 0.20.0 — Build 30

- Bounded BIOS/OS legacy semaphore handshake using byte-wide OS ownership writes, extended capability chain validation, firmware SMI enable rejection and timeout without forced takeover. Not connected to physical PCI or DMA activation.
- 5,000 cooperative firmware handoffs, timeout, malformed chains, SMI gate and byte-only ownership writes (UBSan).

# Development target 0.20.0 — Build 29

- Read-only PCI xHCI BAR validation against an explicitly supplied trusted resource, supervisor-only NX UC mapping in an isolated kernel PML4 branch, framebuffer alias rejection and bounded register callbacks. No BAR probing writes, controller reset, bus mastering or DMA activation.
- PCI resource validation and 10,000 bounded MMIO operations, UC/NX/supervisor PTE checks, physical alias and OOM rollback (UBSan).

# Development target 0.20.0 — Build 28

- Kernel PMM/VM contiguous physical page leases with bounded alignment, generation tokens, pinning against ordinary page release, zeroing and explicit quiescence gate. No device activation or IOMMU setup.
- 10,000 DMA lease cycles, stale-token rejection, pinning, halt gate, fragmentation, table exhaustion and supervisor-only mapping (UBSan).

# Development target 0.20.0 — Build 27

- End-to-end single-interface USB2 boot-HID session: bounded DMA preflight, explicit root-port reset, Address Device, EP0 packet probe, USB enumeration, interrupt endpoint configuration and nonblocking input. All MMIO and DMA accesses are through a model in tests; no physical backend is activated.
- 1,000 full modeled sessions and 20,000 keyboard reports, full/high speed, 32/64-byte contexts, invalid DMA/port rejection, stop and quarantine (UBSan).

# Development target 0.20.0 — Build 26

- Initial eight-byte device descriptor read and Evaluate Context command to update Full Speed EP0 Max Packet Size without resetting its dequeue state. Strict validation of speed/packet combinations and controller result. No physical hardware backend.
- 4,000 full-speed MPS probes, 8/16/32/64 bytes, both context sizes, invalid size and failed context update (UBSan).

# Development target 0.20.0 — Build 25

- Nonblocking submit/poll bridge for one prepared and configured interrupt IN endpoint, MMIO doorbell, event ring, keyboard/mouse boot decoding, report wiping, controller stop and DMA quarantine. Exclusive controller event ownership is required; physical backend remains disabled.
- 20,000 modeled keyboard/mouse reports, malformed and short reports, pending idle polls, abort and DMA quarantine (UBSan).

# Development target 0.20.0 — Build 24

- Single outstanding interrupt IN Normal TRB per endpoint, bounded report buffer, validated event identity and length, wrap and cycle transitions, fail-stop. No physical hardware backend.
- 40,000 modeled interrupt transfers, ring wrap, short IN, malformed events, ownership and bounds (UBSan).

# Development target 0.20.0 — Build 23

- Configure Endpoint command, USB2 interrupt-IN endpoint context generation, interval and packet validation, and DMA ring overlap rejection. Caller supplies initialized pinned coherent rings; physical backend remains disabled.
- 4,000 modeled configure/disable lifecycles, 32/64-byte contexts, DCI 31, interval, DMA overlap and failed endpoint state (UBSan).

# Development target 0.20.0 — Build 22

- Explicit root-port protocol discovery via xHCI extended capabilities, strict USB2 standard-speed profile, bounded power-on/debounce and port reset, preserving unrelated PORTSC change bits. No hardware backend is enabled.
- 10,000 USB2 reset cycles, power/debounce, W1C preservation, USB3/PSI rejection, disconnect/timeout/overcurrent (UBSan).

# Development target 0.20.0 — Build 21

- Single-slot Enable Slot, Address Device and Disable Slot commands; 32/64-byte input/output context layout, EP0 context, DCBAA ownership and address completion validation. Requires an enabled root port and caller-owned DMA areas. Hardware binding remains absent.
- 4,000 slot enable/address/disable lifecycles, 32/64-byte contexts, DMA overlap and invalid slot/output rejection (UBSan).

# Development target 0.20.0 — Build 20

- Control endpoint bridge joining USB enumeration, xHCI transfer and event rings, MMIO doorbell polling and bounded DMA bounce buffers. Model assumes an already-addressed slot and an EP0 context programmed to this ring. No hardware backend is enabled.
- Integrated enumeration of two HID interfaces, 10,000 read transactions, OUT and short IN, fault/timeout and DMA retention on failed halt (UBSan).

# Development target 0.20.0 — Build 19

- Single in-flight endpoint-zero Setup/Data/Status transfer descriptor ring, staged publication, DMA bounds, short IN tracking and transfer-event validation. Endpoint context and slot creation remain unimplemented.
- 60,000 control transactions, setup/data/status, ring wrap, short IN packets, DMA bounds and poisoned endpoints (UBSan).

# Development target 0.20.0 — Build 18

- USB boot-HID enumeration control requests and bounded state machine, including device/configuration descriptor validation, configuration and boot protocol selection. Uses a callback-based control endpoint only; no physical USB transfers.
- 50,000 descriptor mutations, exact control sequence, short transfers, fail-stop, header consistency and boot-only composite policy (UBSan).

# Development target 0.20.0 — Build 17

- Bounded callback-based MMIO controller lifecycle: capability validation, stop/reset, 4 KiB page support check, DCBAA/CRCR/ERST/ERDP programming, polling-mode start, No-op doorbell and completion, and bounded stop. Only a mock backend is provided; real PCI ownership, MMIO mapping and DMA are not enabled.
- 10,000 modeled doorbell/completion exchanges, reset/run/stop, timeout, invalid completion, DMA quarantine and unsupported capability rejection (UBSan).

# Development target 0.20.0 — Build 16

- Bounded 64-entry command/event TRB rings, No-op command submission, cycle transitions, FIFO completion pointer validation, and event dequeue accounting. The kernel exposes a software-only ring self-test; no controller is activated.
- 124,000 command submissions/completions, full-ring backpressure, wrap and cycle changes, invalid completion pointers and error completion (UBSan).

# Development target 0.20.0 — Build 15

- Add EXT4 superblock UUID extraction and reject zero-UUID volumes.
- Key SwampGuard ceilings by the full 128-bit EXT4 UUID plus inode, eliminating cross-volume inode collisions.
- Preserve a profile when the same UUID volume is unmounted and remounted; different UUIDs receive independent profiles.
- Add host tests for duplicate inode numbers across volumes, remount stability, profile removal isolation and zero-UUID rejection.
- Keep policies volatile; cloned volumes with the same UUID intentionally share policy until UUID enrollment/management is implemented.

# Development target 0.20.0 — Build 14

- Add a resumable single-client Ring 3 GUI execution path, trusted register snapshot, CR3 restore, yield and two-tick timer quantum, 100-slice budget.
- Animate 24 frames in Paint, cancel with Shift+X or window close; recheck policy revision before every resume, cancel on remount or policy change.
- Retain synchronous capture and reject concurrent native execution while a GUI client is paused. Add `gui-async-test` and 2,000 mock CPU session tests.
- Host-only validation; no claim of actual guest CPU resume or hardware support.

# Development target 0.20.0 — Build 13

- Add a real graphical editor for the six bundled ELF application capability ceilings, with staged changes, explicit apply, reload and deny-all.
- Share the audited policy update path with the trusted terminal and invalidate stale drafts on policy and mount changes.
- Keep the audit view, shell commands and all previous source paths; correct the Live boot checkpoint label.
- Integrate key/mouse and actual EXT4 policy tests; 50,000 UI input cases and framebuffer canaries.

# Development target 0.20.0 — Build 12

- Add 32 volatile per-inode launch capability ceilings and trusted shell management commands.
- Check explicit grants on all storage ELF launch routes, including group and GUI capture; audit denial and policy edits.
- Test 100,000 policy combinations and real EXT4 command dispatch with mock CPU entry.

# Development target 0.20.0 — Build 11

- Increase selected-resource ceiling to 64 KiB; preserve 256-byte transfer and legacy run-data limit.
- Add native resource-scan, SDK limits, EXT4 fixture and host/dispatcher tests.
- Preserve existing GUI, boot menu, Live disk isolation and all source paths.

# Development target 0.20.0 — Build 10

- Add explicit per-run resource selection for a single RAMFS or EXT4 file.
- Separate CAP_DATA and optional CAP_CONSOLE launch commands; preserve previous commands and ABI.
- Validate source/path grammar, regular file type and 256-byte cap; clear staging and session data.
- Add 50,000 snapshot/session cases and pending guest integration assertions.

# Development target 0.20.0 — Build 09

- Extract read-only ATA transport into a testable driver with LBA48 addressing, validated 512-byte logical sectors and bounded polling.
- Preserve the secondary IDE master lab interface; prohibit the hardware wrapper in Live mode.
- Add `ata-info` and 50,000 IDENTIFY mutation cases, exact task-file order and device fault tests.

# Development target 0.20.0 — Build 08

- Expand RAMFS to 64 × 8192-byte files and 64 directories.
- Add append, bounded offset reads and atomic preflight for moving directory subtrees.
- Add explicit read-only EXT4 import and shell commands.
- Test 40,000 subtree moves, rejected path/depth/collision cases and actual EXT4 import.

# Development target 0.20.0 — Build 07

- Classify UHCI, OHCI, EHCI and xHCI PCI functions; add read-only `usb` inventory.
- Add bounded USB boot interface discovery and keyboard/mouse report codecs; no USB transfers.
- Add legacy RTC stable snapshot, BCD/binary and 12/24 hour decoding; `rtc` shell command.
- Add 200,000 mutation/report cases with UBSan; keep existing PS/2 input unchanged.

# Development target 0.20.0 — Build 06

- Add capability-gated one-shot native GUI publishing syscall (11), SDK API and standalone `/bin/gui-demo` ELF.
- Validate 128 palette indices and user address ranges, commit the captured buffer only on clean exit, scrub staging state.
- Add Paint Shift+G demo import, shell `gui-demo`, EXT4 packaging and guest assertions.
- Add 2,000 dispatcher simulations and exact EXT4 ELF read-back; full regression passed.

# Development target 0.20.0 — Build 05

- Add a bounded directory hierarchy to volatile RAMFS, including canonical relative paths, parent checks and file operations across directories.
- Add `rammkdir`, `ramrmdir`, `ramdirs` and scoped `ramls <dir>`; retain flat commands.
- Reject nonempty directory removal and file/directory path collisions; run 50,000 nested file cycles under UBSan.

# Development target 0.20.0 — Build 04

- Add five kernel-backed tools: RAM browser, HEX inspector, pixel canvas, theme settings and read-only audit viewer.
- Expand window manager to ten windows; two-column launcher and group taskbar.
- Save/load bounded indexed-color paint files in volatile RAMFS; reject malformed files.
- Retain classic five-view desktop, all previous paths and separate locked boot route.

# Development target 0.20.0 — Build 03

- Expanded BIOS kernel load geometry to 256 KiB with shared constants and EDD checks.
- Added PS/2 extended arrows and 16-entry shell history.
- Expanded guest integration gate with RAMFS lifecycle assertions.
- Two clean builds produced identical checksums for all 20 distributed artifacts.

# Development target 0.20.0 — Build 02

- Added bounded volatile RAMFS, native shell commands and Notes Save/Load.
- Retained original EXT4 reader and isolated protected boot image.
- Clean build and full host regression passed.

# Development target 0.20.0 — Build 01

- Added bounded window manager and host regression tests.
- Preserved classic Farkuad UI as boot option 2.
- Optimized selected kernel files to retain BIOS image size limit.

# Changelog

## [0.13.1] Farkuad — input and boot menu

- Add an ASCII ogre UEFI boot menu, four real options and a 10-second default timeout.
- Initialize i8042 keyboard and auxiliary ports explicitly, with bounded ACK/resend handling.
- Avoid keyboard starvation by absent COM1; poll keyboard and mouse before optional serial.
- Add relative PS/2 mouse cursor, tab selection and file selection.
- Add mock firmware/controller tests and framebuffer bounds coverage; guest validation pending.

## [0.13.0 ISO revision] Farkuad

- Replace the primary Live IMG with a deterministic 81 MiB hybrid ISO9660 image.
- El Torito EFI/FAT16 optical boot and MBR/FAT32 USB boot share an identical EFI payload.
- Update build, flash, checksum, CI, Git LFS ignore exceptions and documentation.
- Add independent structure and payload extraction tests and an optical guest gate.
- No kernel feature changes; firmware execution remains pending.

## [0.13.0] Farkuad — first Live preview

### Added
- Kernel-mode framebuffer desktop with terminal, notes, calculator, monitor and read-only EXT4 file previews.
- Separate `FARKUAD.EFI` with embedded kernel and 8 MiB EXT4 RAM disk.
- Removable-media 65 MiB MBR/FAT32 raw image, UEFI fallback path.
- Supervisor-only high framebuffer mapping with NX and rollback; framebuffer metadata validation.
- RAM-disk block device and firmware usable-range overlap checks.
- Live build and guarded Linux flashing scripts; SHA-256 and guest USB boot gate.
- New host tests, renderer preview, Live documentation and release branding.

### Changed
- Live physical ATA/GPT mounting and VirtIO network lab activation disabled; existing other lab paths retained.
- Selected kernel compilation units optimized for size to preserve the existing 60 KiB BIOS payload limit.
- FAT32 writer allows a larger payload only when explicitly requested by the Live builder.
- Git LFS image checksum tracked, CI uploads Farkuad artifacts.

### Pending
- Actual QEMU/OVMF and physical hardware execution, USB HID, mouse, persistent filesystem, signed boot, SwampLogin authentication and encrypted volumes, independent security audit.

Previous 0.12.0 changelog: `legacy/CHANGELOG-0.12.0.md`.


# Development target 0.20.0 — Build 31

- Replace fixed 16 MiB optical FAT16 and 64 MiB USB FAT32 reservations with payload-sized volumes.
- Enforce FAT16/El Torito 16-bit sector-count constraints and cap EFI payload at 30 MiB.
- Enforce FAT32 minimum cluster count, add a 4 MiB payload reserve and 1 MiB volume alignment.
- Add an actual image utilization report for kernel, EXT4, EFI, FAT16, FAT32 and total ISO sizes.
- Add growth tests proving larger EFI payloads produce larger hybrid ISO and raw USB images.
- Preserve the existing UEFI payload and locked SwampBoot artifacts.


# Development target 0.20.0 — Build 32

- Added read-only runtime xHCI PCI inventory command `xhci`.
- Validates PCI class `0C:03:30`, BDF identity, endpoint header and BAR0 type.
- Decodes 32-bit and 64-bit memory BAR0 and reports unassigned BAR state.
- Reports PCI Command Register I/O Space, Memory Space and Bus Master bits.
- Does not write PCI configuration space, probe BAR size, enable Bus Master, map MMIO or activate DMA.
- Updated kernel version banner and UEFI boot menu to DEV 32.
- Retained Build 31 dynamic Live ISO sizing and all historical logs.


# Development target 0.20.0 — Build 33

- Added UEFI `EFI_PCI_IO_PROTOCOL` enumeration for xHCI PCI functions.
- Added bounded `GetBarAttributes` QWord/DWord memory-resource parsing and `FreePool` cleanup.
- Added versioned xHCI resource records to the UEFI boot information page.
- Added kernel `xhci-fw` command and live PCI configuration cross-check of BDF, IDs, class, BAR0, width and PCI Command flags.
- Added protocol ABI compile-time offset/size assertions.
- No PCI attribute changes, BAR writes, MMIO access, Bus Master enable or DMA.


# Development target 0.20.0 — Build 34

- Added kernel `xhci-probe` read-only xHCI capability register probe.
- Added firmware-record selection and live PCI revalidation before MMIO mapping.
- Added E820/UEFI physical-range alias protection to the MMIO mapper.
- Extended UEFI handoff with a complete non-MMIO physical-range map, including high RAM and ACPI ranges.
- Fail-closed when the physical-range map is incomplete or exceeds 64 entries.
- Capability probe does not write xHCI registers, reset/start the controller, enable Bus Master or perform DMA.


# Development target 0.20.0 — Build 35

- Added ACPI DMAR table parser for Intel VT-d DRHD, RMRR, ATSR/SATC structures and device scopes.
- Added bounded DRHD/scope inventory with malformed/truncated fail-closed state.
- Added `iommu` command with xHCI segment/BDF coverage checks using include-all or exact one-hop endpoint scope.
- Live PCI identity is revalidated before reporting xHCI coverage.
- No IOMMU registers, remapping tables, PCI attributes or DMA are changed. AMD IVRS remains unsupported.
