# Current USB status — Build 37

The Q35 Live profile now drives one or two boot HID devices after UEFI exit. Keyboard and mouse ran together with device-scoped VT-d translation. Physical controllers remain a separate validation target. See [current implementation and commands](USB-INPUT.md).

## Build 35 record

### Evidence at Build 35

Build 27 demonstrates a single USB2 boot-HID session from controller reset to decoded keyboard reports through an injected controller model. Builds 28–30 add independently tested pinned physical-page leases, a kernel-only UC MMIO mapping and bounded register callbacks, validation against an independently supplied PCI resource, and a cooperative firmware ownership handshake. These primitives are compiled into the kernel; there is still no physical backend invocation at boot or in the desktop. PS/2 remains the only hardware input path exercised in previous Farkuad VM feedback.

## Remaining physical-backend gates

- Obtain authoritative assigned PCI BAR resource lengths and memory-map exclusions; do not infer aperture size from a BAR address. Map and validate the actual UC aperture on guest hardware.
- Establish PCI command ownership and controlled bus mastering, device DMA containment (IOMMU or explicitly isolated disposable VM), and a calibrated deadline source. Integrate firmware handoff without forcibly taking ownership; reject firmware SMI enables that remain set. Support or reject scratchpads, custom protocol speed IDs, hubs and SuperSpeed explicitly.
- Connect the allocator, MMIO window, ownership protocol and single USB session in an explicit opt-in backend. Handle disconnect, timeout, controller halt and DMA quarantine before releasing any buffers. Connect events to desktop input, including multiple interfaces where supported.
- Execute guest tests with USB keyboard and mouse and PS/2 fallback, then hardware validation. No QEMU or VirtualBox executable is available in this build environment.

## Historical checkpoint notes (preserved)

# xHCI transport development status — Build 22

## Implemented and host-tested

- USB2 root-port capability discovery, bounded power/debounce/reset.
- Controller halt/reset/start/stop via an injected MMIO backend.
- Command/event rings, completion pointer and slot checks, one configured device slot.
- 32/64-byte address contexts and DCBAA ownership.
- EP0 Setup/Data/Status ring and bounded 4 KiB DMA bounce-buffer bridge.
- USB device/config descriptor enumeration and Boot HID protocol selection.
- Keyboard/mouse boot report codecs (Build 07).

These are separately tested components. The Build 20 test links enumeration through the real EP0 transfer code to a modeled controller and an already-addressed slot. The Build 21 and 22 tests exercise slot commands and root-port reset separately. A single combined real-hardware enumeration path has not been demonstrated.

## Remaining gates before claiming physical USB input

1. Discover the PCI BAR and trusted aperture, implement UC MMIO mapping, BIOS/OS ownership handoff, PCI command gating, coherent DMA allocation and pinning, and IOMMU policy. No real DMA may be enabled automatically in Live.
2. Support required scratchpads and validated protocol speed IDs, or reject unsupported controllers before activation. Add Evaluate Context for changing full-speed EP0 max packet size when needed.
3. Configure the interrupt IN endpoint contexts and rings; implement periodic boot HID polling, disconnect teardown and error recovery. Only then forward reports to the existing keyboard/mouse event path.
4. Exercise the entire sequence in QEMU/OVMF or VirtualBox with disposable USB devices, check guest boot and PS/2 regression, then test on isolated physical hardware. No claim of working USB HID before these gates pass.

Scope remains one directly attached USB 2.0 boot-HID device, not SuperSpeed, hubs, storage or arbitrary classes. This release's live hardware path continues to use PS/2.

## Build 23 delta

USB2 interrupt IN context generation and the Configure Endpoint command now pass modeled slot lifecycle tests. Endpoint data transport and hardware mapping remain pending; caller-owned transfer rings must be initialized and pinned before configuration. No physical DMA activation.

## Build 24 delta

Single outstanding interrupt IN Normal TRB per endpoint, cycle/link wrap, strict transfer completion matching and short packet accounting. No endpoint data doorbell bridge or real HID events yet. `xhci-interrupt-test` is software-only.

## Build 25 delta

The single-endpoint interrupt IN bridge now submits doorbells and polls the event ring without blocking. It decodes keyboard or mouse boot reports through existing HID codecs and quarantines DMA if controller halt fails. The test starts the controller and addresses/configures the device through a model. No real PCI backend, simultaneous HID polling or desktop event injection exists yet. The caller must exclusively own the controller event ring while a transfer is pending and explicitly abort before releasing DMA.

## Build 26 delta

Full-speed EP0 discovery now reads the initial eight bytes of the device descriptor and issues Evaluate Context to update Max Packet Size to 8, 16, 32 or 64 bytes if necessary. Low/high-speed mismatches fail closed. Actual hardware remains untested. Clean rebuild completed.

## Build 27 delta

Single-interface USB2 boot-HID session orchestration integrates root-port reset, Address Device, initial EP0 probe, enumeration, Configure Endpoint, and nonblocking input. The integrated controller model passes 1,000 full sessions and 20,000 keyboard reports; the model covers full/high speed and both context strides. Only one HID interface per session is supported; desktop input routing, actual PCI mapping/ownership and physical DMA still pending.

## Build 28 delta

The kernel PMM now provides generation-tagged, aligned contiguous DMA leases. Ordinary page release cannot free pinned pages; VM wrappers clear allocation/release memory and require a caller-confirmed DMA quiescence gate. This is a memory-lifetime mechanism, not proof of device isolation; no IOMMU setup or physical bus mastering is performed.

## Build 29 delta

The VM provides one permanent, supervisor-only, NX UC MMIO mapping in a separate kernel-root PML4 branch. PAT UC entry and CPU physical address width are checked on x86; the host test verifies page tables and OOM rollback. The PCI helper validates an assigned xHCI BAR against an independently trusted resource descriptor, does not size/probe BARs by writes, and supplies bounded MMIO callbacks. Actual firmware resource assignment and physical controller binding are still missing; mapping is not invoked by Live boot or user commands.


## Build 30 delta

The ownership helper walks the bounded xHCI extended capability chain, requests the OS semaphore using a byte-wide write, waits at most 1000 ms, checks BIOS release and rejects still-enabled firmware SMI. It does not write BIOS ownership, force a timeout takeover, or modify SMI enable/status registers. The byte-wide write avoids overwriting the adjacent BIOS semaphore. This matches the ownership field layout described in the xHCI specification (sections 7.1.1–7.1.2); see also Linux's public field definitions for cross-checking. Implementation is original, not imported Linux driver code.

- xHCI specification: https://cdrdv2-public.intel.com/625472/625472_xHCI_Rev1_2b.pdf
- Linux field reference: https://github.com/torvalds/linux/blob/master/drivers/usb/host/xhci-ext-caps.h


## Build 32 delta

The kernel now has a read-only `xhci` terminal inventory command. It uses the existing PCI mechanism-1 configuration reader to scan functions with class `0C:03:30`, revalidates vendor/device/class/header fields, decodes BAR0 as 32-bit or 64-bit memory BAR, and reports PCI Command Register I/O Space, Memory Space and Bus Master flags. It does not write PCI configuration space, probe BAR size, enable decoding or bus mastering, map MMIO, claim firmware ownership or perform DMA. The command is useful for discovering the controller and its assigned BAR address, but the BAR aperture length still requires an independent trusted resource source. Physical backend remains disabled.


## Build 33 delta — firmware BAR resource metadata

Farkuad's UEFI loader queries `EFI_PCI_IO_PROTOCOL` for xHCI functions and calls `GetBarAttributes` for BAR0. The bounded parser accepts a single matching QWord or DWord memory descriptor, zero address translation, a page-aligned BAR base and aperture, and a maximum 2 MiB length. It validates the EndTag/checksum, supports optional trailing ResourceSource fields, and frees firmware-allocated descriptor buffers with Boot Services `FreePool`.

A versioned record is added to the reserved boot information page. `xhci-fw` validates record bounds and cross-checks segment 0 BDF, vendor/device, class, BAR0 address/type and PCI Command flags against live PCI configuration space. Nonzero segments are skipped until ECAM exists. The record is firmware-reported metadata only: no BAR writes, PCI attribute changes, MMIO or DMA. Actual firmware and physical-hardware behavior remains unverified.


## Build 34 delta — read-only capability probe

`xhci-probe` uses the versioned UEFI resource handoff, revalidates PCI identity/BAR/command state, requires Memory Space already enabled, maps BAR0 through the UC/NX/supervisor-only MMIO mapper and reads only capability registers via `xhci_caps_read`. It does not write operational registers or start the controller.

The MMIO mapper now rejects overlap with active physical-memory ranges. UEFI handoff retains all non-MMIO memory descriptors in a separate map (high RAM, loader/runtime, ACPI and persistent memory); UEFI MMIO descriptors are excluded. If this map exceeds 64 entries or is incomplete, MMIO mapping is refused.


## Build 35 delta — IOMMU topology preflight

The ACPI parser now inventories Intel VT-d DMAR structures. The `iommu` command reports DRHD units, RMRR regions, device scopes and include-all flags, then checks whether a live-revalidated xHCI BDF is covered by a matching segment's include-all DRHD or an exact one-hop endpoint scope. Multi-hop paths are reported as not proven. This does not enable VT-d or authorize DMA. AMD IVRS is not implemented.
