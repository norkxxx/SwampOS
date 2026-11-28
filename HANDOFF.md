# Development handoff — target SwampOS 0.20.0

The newest cumulative ZIP is the only required working source; keep Farkuad 0.13.1 separately as the verified fallback. Do not publish intermediate builds as finished releases. Retain all original source/doc paths, compare against the original ZIP using `scripts/checkpoint-report.py`, and run `make test` plus `make checksums` before packaging. Include the hybrid ISO, styled English README, banners, RELEASE.md, CHANGELOG.md, FEATURES.md, MISSING.md, DIFF-REPORT.md, TREE.txt and test logs. Use Git LFS and give a commit command at the end of each report.

## Current checkpoint · 0.20.0-dev.38-r1

The AHCI and NVMe profiles mount one fixed file, `/persist/note`, through the block/VFS layer. Both transports survived an abrupt QEMU shutdown and recovered the preceding revision after on-disk corruption. The cumulative ZIP retains prior build contents. Read docs/PERSISTENCE.md, TEST-REPORT.md and the guest logs.

Next milestone is Build 39, SwampGuard and user-process permissions. The storage fixture is suitable for CRC-checked user text. Persisting trusted policy requires identity, authentication, controlled updates and rollback handling. Multiple DMA domains, dynamic files and writable EXT4 remain open.

## Build 37 handoff (historical)

Build 37 boots the cumulative Live desktop and adds native xHCI keyboard/mouse input in an opt-in Q35/OVMF profile. The kernel owns two HID slots and a restricted, legacy VT-d domain. The QEMU test disables i8042, types `version` on USB, drags and resizes Notes by USB mouse, then inspects the DMA tables independently. See docs/USB-INPUT.md and TEST-REPORT.md. The ISO hash is `d480878ebf9de1b3400407a4bb15d7a3142412d984fd0bfffebeee6e1942594a`.

The next planned subsystem is storage. Start with controller discovery and the existing block/VFS contracts, then exercise AHCI/NVMe on disposable disk images. Keep the Live root in RAM and preserve the disk-write boundary. Remaining USB work includes hubs, physical platform ownership and hardware qualification. The VirtualBox flicker retest remains open.

## Build 36 handoff (historical)

The integrated desktop renderer is in `desktop/compositor.c`. The production callers are in `kernel/desktop.c`. The clean build, aggregate host suite and six QEMU/OVMF routes are recorded in TEST-REPORT.md and build/logs. The Live ISO SHA-256 is `526cff3308caf630d832c6117f627a2e5085c2dea43fa1ee4031300829523d45`.

The next planned checkpoint is xHCI USB input with verified device isolation. Start from the Q35 `live-iommu-runtime` fixture. It exercises discovery, BAR validation, capability reads and DMAR scope matching. The guest input path in this checkpoint uses PS/2. Per-device DMA remapping, active kernel-owned xHCI and physical USB HID still require implementation and tests. The VirtualBox checklist for the reported flicker is in docs/VM-ACCEPTANCE.md.

## Build 35 handoff (historical)

Build 35 covered, Intel VT-d DMAR Inventory. Build 34 read-only xHCI capability probe remains included. Build 33 UEFI PCI BAR resource handoff remains included. Build 32 runtime xHCI PCI inventory remains included. The kernel terminal commands `xhci` and `xhci-fw` report live PCI identity and firmware-reported BAR0 resources; `xhci-fw` revalidates segment-0 BDF, IDs, class, BAR0, width and PCI Command flags against live PCI config space. `xhci-probe` performs read-only xHCI capability-register reads through the guarded UC mapping; `iommu` parses Intel DMAR and reports only proven include-all/direct endpoint coverage. The current kernel/UEFI banner is DEV 35 (Build 34 used DEV 34). Build 31 dynamic Live ISO sizing remains in place. The optical FAT16 and USB FAT32 volumes are sized from the EFI payload; El Torito/FAT16 limits are enforced with a 30 MiB EFI ceiling, FAT32 minimum cluster count is enforced, and a component utilization report is generated. Current Build 35 ISO size is recorded in `build/Farkuad-Live.iso.report`. Exact EFI extraction from both volumes and growth tests pass. Build 35 `live-host-test` and `boot-host-test` completed successfully; aggregate `make test` was not run to completion. QEMU/VirtualBox/OVMF remain unavailable. Actual UEFI `GetBarAttributes`, physical xHCI capability reads, DMAR firmware inventory and USB input validation are pending. Previous Build 30, Explicit Firmware Ownership. Bounded BIOS/OS legacy semaphore handshake using byte-wide OS ownership writes, extended capability chain validation, firmware SMI enable rejection and timeout without forced takeover. Not connected to physical PCI or DMA activation. Previous Build 29, PCI BAR and UC MMIO Mapping. Read-only PCI xHCI BAR validation against an explicitly supplied trusted resource, supervisor-only NX UC mapping in an isolated kernel PML4 branch, framebuffer alias rejection and bounded register callbacks. No BAR probing writes, controller reset, bus mastering or DMA activation. Previous Build 28, Pinned DMA Memory. Kernel PMM/VM contiguous physical page leases with bounded alignment, generation tokens, pinning against ordinary page release, zeroing and explicit quiescence gate. No device activation or IOMMU setup. Previous Build 27, USB2 HID Boot Session. End-to-end single-interface USB2 boot-HID session: bounded DMA preflight, explicit root-port reset, Address Device, EP0 packet probe, USB enumeration, interrupt endpoint configuration and nonblocking input. All MMIO and DMA accesses are through a model in tests; no physical backend is activated. Previous Build 26, Full-Speed EP0 Probe. Initial eight-byte device descriptor read and Evaluate Context command to update Full Speed EP0 Max Packet Size without resetting its dequeue state. Strict validation of speed/packet combinations and controller result. No physical hardware backend. Previous Build 25, Nonblocking HID Transport. Nonblocking submit/poll bridge for one prepared and configured interrupt IN endpoint, MMIO doorbell, event ring, keyboard/mouse boot decoding, report wiping, controller stop and DMA quarantine. Exclusive controller event ownership is required; physical backend remains disabled. Previous Build 24, xHCI Interrupt Transfer Ring. Single outstanding interrupt IN Normal TRB per endpoint, bounded report buffer, validated event identity and length, wrap and cycle transitions, fail-stop. No physical hardware backend. Previous Build 23, xHCI Interrupt Endpoint Contexts. Configure Endpoint command, USB2 interrupt-IN endpoint context generation, interval and packet validation, and DMA ring overlap rejection. Caller supplies initialized pinned coherent rings; physical backend remains disabled. Previous Build 22, USB2 Root Port Reset. Explicit root-port protocol discovery via xHCI extended capabilities, strict USB2 standard-speed profile, bounded power-on/debounce and port reset, preserving unrelated PORTSC change bits. No hardware backend is enabled. Previous Build 21, xHCI Device Context & Slot Lifecycle. Single-slot Enable Slot, Address Device and Disable Slot commands; 32/64-byte input/output context layout, EP0 context, DCBAA ownership and address completion validation. Requires an enabled root port and caller-owned DMA areas. Hardware binding remains absent. Previous Build 20, Integrated xHCI EP0 Transport. Control endpoint bridge joining USB enumeration, xHCI transfer and event rings, MMIO doorbell polling and bounded DMA bounce buffers. Model assumes an already-addressed slot and an EP0 context programmed to this ring. No hardware backend is enabled. Previous Build 19, xHCI EP0 Control TRBs. Single in-flight endpoint-zero Setup/Data/Status transfer descriptor ring, staged publication, DMA bounds, short IN tracking and transfer-event validation. Endpoint context and slot creation remain unimplemented. Previous Build 18, USB Enumeration Protocol. USB boot-HID enumeration control requests and bounded state machine, including device/configuration descriptor validation, configuration and boot protocol selection. Uses a callback-based control endpoint only; no physical USB transfers. Previous Build 17, xHCI Controller Lifecycle. Bounded callback-based MMIO controller lifecycle: capability validation, stop/reset, 4 KiB page support check, DCBAA/CRCR/ERST/ERDP programming, polling-mode start, No-op doorbell and completion, and bounded stop. Only a mock backend is provided; real PCI ownership, MMIO mapping and DMA are not enabled. Previous Build 16, xHCI Ring Transport. Bounded 64-entry command/event TRB rings, No-op command submission, cycle transitions, FIFO completion pointer validation, and event dequeue accounting. The kernel exposes a software-only ring self-test; no controller is activated. Previous Build 15, SwampGuard ceilings keyed by EXT4 UUID + inode (different UUIDs isolate same inode number; same-volume remount retains policy; duplicate UUID clones share policy; policies remain volatile). Build 14, one resumable native GUI client with bounded timer/yield slices, Paint animation and cancellation on policy/mount revision; actual IRETQ resume guest-pending. Build 13, graphical SwampGuard ceiling editor with explicit apply and revision checks, Build 12, volatile launch capability ceilings and audit, Build 11, 64 KiB read-only selected resources and native resource-scan, Build 10, explicit per-run selected file snapshots with separate console grant, Build 09, read-only ATA LBA48 transport (non-Live lab only), expanded RAMFS, subtree moves and explicit read-only EXT4 import, USB controller inventory and boot HID codecs, legacy RTC, one-shot native GUI capture, ten kernel desktop tools, window manager, hierarchical volatile RAMFS, enlarged BIOS payload and command history. Next priorities: real guest verification (VirtualBox/QEMU/OVMF), physical xHCI controller activation, IOMMU remapping and DMA isolation, keyboard/mouse integration, general GUI client separation, persistent signed policy storage and a larger writable filesystem. Build 31 fixes ISO sizing; Build 32 adds read-only PCI inventory; Build 33 adds UEFI-reported BAR0 resource metadata and live PCI revalidation; Build 34 adds read-only xHCI capability reads through a guarded UC mapping. None of these builds starts the controller or activates physical USB transfers. Then continue the original security, networking, package, compatibility and compute roadmap. No fixed build count; use actual acceptance evidence, not estimates of completion. Never claim a test was executed when its tool was unavailable.


## Build 16 — xHCI Ring Transport

Bounded 64-entry command/event TRB rings, No-op command submission, cycle transitions, FIFO completion pointer validation, and event dequeue accounting. The kernel exposes a software-only ring self-test; no controller is activated.


## Build 17 — xHCI Controller Lifecycle

Bounded callback-based MMIO controller lifecycle: capability validation, stop/reset, 4 KiB page support check, DCBAA/CRCR/ERST/ERDP programming, polling-mode start, No-op doorbell and completion, and bounded stop. Only a mock backend is provided; real PCI ownership, MMIO mapping and DMA are not enabled.


## Build 18 — USB Enumeration Protocol

USB boot-HID enumeration control requests and bounded state machine, including device/configuration descriptor validation, configuration and boot protocol selection. Uses a callback-based control endpoint only; no physical USB transfers.


## Build 19 — xHCI EP0 Control TRBs

Single in-flight endpoint-zero Setup/Data/Status transfer descriptor ring, staged publication, DMA bounds, short IN tracking and transfer-event validation. Endpoint context and slot creation remain unimplemented.


## Build 20 — Integrated xHCI EP0 Transport

Control endpoint bridge joining USB enumeration, xHCI transfer and event rings, MMIO doorbell polling and bounded DMA bounce buffers. Model assumes an already-addressed slot and an EP0 context programmed to this ring. No hardware backend is enabled.


## Build 21 — xHCI Device Context & Slot Lifecycle

Single-slot Enable Slot, Address Device and Disable Slot commands; 32/64-byte input/output context layout, EP0 context, DCBAA ownership and address completion validation. Requires an enabled root port and caller-owned DMA areas. Hardware binding remains absent.

Builds 16–22 implement host-tested xHCI transport components; read `docs/USB-XHCI-STATUS.md` for precise acceptance gaps. Prioritize a verified PCI/MMIO/DMA backend, full-speed EP0 context evaluation, interrupt endpoint configuration and guest integration instead of adding more disconnected mock codecs. The release remains an internal development checkpoint, not final 0.20.0.


## Build 22 — USB2 Root Port Reset

Explicit root-port protocol discovery via xHCI extended capabilities, strict USB2 standard-speed profile, bounded power-on/debounce and port reset, preserving unrelated PORTSC change bits. No hardware backend is enabled.


## Build 23 — xHCI Interrupt Endpoint Contexts

Configure Endpoint command, USB2 interrupt-IN endpoint context generation, interval and packet validation, and DMA ring overlap rejection. Caller supplies initialized pinned coherent rings; physical backend remains disabled.


## Build 24 — xHCI Interrupt Transfer Ring

Single outstanding interrupt IN Normal TRB per endpoint, bounded report buffer, validated event identity and length, wrap and cycle transitions, fail-stop. No physical hardware backend.


## Build 25 — Nonblocking HID Transport

Nonblocking submit/poll bridge for one prepared and configured interrupt IN endpoint, MMIO doorbell, event ring, keyboard/mouse boot decoding, report wiping, controller stop and DMA quarantine. Exclusive controller event ownership is required; physical backend remains disabled.


## Build 26 — Full-Speed EP0 Probe

Initial eight-byte device descriptor read and Evaluate Context command to update Full Speed EP0 Max Packet Size without resetting its dequeue state. Strict validation of speed/packet combinations and controller result. No physical hardware backend.


## Build 27 — USB2 HID Boot Session

End-to-end single-interface USB2 boot-HID session: bounded DMA preflight, explicit root-port reset, Address Device, EP0 packet probe, USB enumeration, interrupt endpoint configuration and nonblocking input. All MMIO and DMA accesses are through a model in tests; no physical backend is activated.


## Build 28 — Pinned DMA Memory

Kernel PMM/VM contiguous physical page leases with bounded alignment, generation tokens, pinning against ordinary page release, zeroing and explicit quiescence gate. No device activation or IOMMU setup.


## Build 29 — PCI BAR and UC MMIO Mapping

Read-only PCI xHCI BAR validation against an explicitly supplied trusted resource, supervisor-only NX UC mapping in an isolated kernel PML4 branch, framebuffer alias rejection and bounded register callbacks. No BAR probing writes, controller reset, bus mastering or DMA activation.


## Build 30 — Explicit Firmware Ownership

Bounded BIOS/OS legacy semaphore handshake using byte-wide OS ownership writes, extended capability chain validation, firmware SMI enable rejection and timeout without forced takeover. Not connected to physical PCI or DMA activation.


## Build 31 — Dynamic Live Image Sizing

`tools/mkiso.py` sizes the El Torito FAT16 image from EFI payload bytes and sizes the USB FAT32 volume using the FAT32 minimum-cluster rule plus reserve. `tools/mklive.py` uses the same FAT32 sizing helper. `tools/image_report.py` writes `build/Farkuad-Live.iso.report`. Tests `mkiso_dynamic_host.py`, `mklive_dynamic_host.py` and `live_image.py` validate growth and exact payload extraction. Current measured Build 35 image is 42.88 MiB.


## Build 32 — Runtime xHCI PCI Inventory

- `drivers/usb/xhci/pci_inventory.c/.h`: read-only PCI class `0C:03:30` inspection, BAR0 32/64-bit memory decoding, command/status reporting and fail-closed validation.
- `kernel/hardware.c`: `xhci` command scans PCI configuration space and prints discovered controllers.
- `kernel/main.c`: help and version string updated to DEV 32.
- `boot/uefi/live_menu.h`: UEFI menu banner updated to DEV 32.
- `tests/xhci_pci_inventory_host.c`: UBSan host tests for valid/invalid functions and BARs.
- Physical xHCI backend remains disabled. No PCI config writes, BAR sizing, MMIO mapping, Bus Master enable, DMA, or HID event routing.
- Clean build passed. Full `make -j8 test` exceeded the 300-second tool limit; preserve the incomplete status. QEMU/OVMF and VirtualBox are unavailable.


## Build 33 — UEFI PCI BAR resource handoff

- `boot/uefi/pci_resource.c/.h`: `EFI_PCI_IO_PROTOCOL` enumeration, QWord/DWord BAR0 resource parsing, bounded descriptor handling, EndTag checksum, `FreePool` cleanup and xHCI record handoff.
- `kernel/bootinfo.h`: versioned xHCI handoff records, fixed ABI size and boot-page fit assertions.
- `kernel/hardware.c`: `xhci-fw` command revalidates firmware records against live PCI config before display.
- `drivers/usb/xhci/pci_inventory.c/.h`: shared `xhci_pci_resource_matches()` live config cross-check.
- `tests/uefi_pci_resource_host.c`: mocked UEFI PCI I/O protocol and resource parser tests.
- Build 33 does not set PCI attributes, write BARs, map MMIO, enable Bus Master or perform DMA. Actual firmware and physical validation remain pending.


## Build 34 — Read-only xHCI capability probe and MMIO alias protection

- `kernel/hardware.c`: `xhci-probe` selects a UEFI resource record, revalidates PCI config, requires Memory Space enabled, maps the aperture and reads capability registers only.
- `kernel/vm.c/.h`: stores physical ranges, rejects RAM/framebuffer aliases, exposes `vm_mmio_matches`, supports idempotent same-window mapping.
- `boot/uefi/handoff.c`: records all non-MMIO UEFI memory ranges separately from the low PMM map; overflow marks the map incomplete.
- `kernel/bootinfo.h`: versioned physical-range map; still fits reserved 4 KiB page.
- `tests/uefi_handoff_host.c`, `tests/xhci_caps_probe_host.c`, and updated `tests/mmio_resource_host.c`.
- No controller reset/start, firmware ownership change, PCI attribute change, Bus Master enable or DMA. Actual physical probe remains pending.


## Build 35 — Intel VT-d DMAR inventory

- `kernel/acpi.c`: checksum-validated DMAR parsing for DRHD, RMRR, ATSR/SATC and device scopes.
- `kernel/hardware.h`: bounded DRHD/scope inventory structures and `acpi_dmar_covers()`.
- `kernel/hardware.c`: `iommu` command; reports Intel VT-d topology and only claims xHCI coverage for a live-revalidated PCI function covered by include-all or exact one-hop endpoint scope.
- `tests/acpi_dmar_host.c`: synthetic RSDP/XSDT/DMAR host tests under UBSan.
- No IOMMU enablement, register programming, remapping tables, PCI changes or DMA. AMD IVRS and multi-hop scope resolution remain pending.
