# Unfinished requirements — target 0.20.0 and beyond

- Build 36 VirtualBox and physical-hardware validation, hardware scanout synchronization, and visual testing across display adapters. QEMU/OVMF USB and optical Live paths passed. Build 31 fixes dynamic Live image sizing; Build 32 adds read-only runtime xHCI PCI inventory; Build 33 adds UEFI-reported BAR0 resource handoff; Build 34 adds a read-only capability probe and E820/UEFI MMIO alias protection; Build 35 adds Intel VT-d DMAR inventory only. Actual firmware/physical USB and IOMMU validation remain pending.
- General asynchronous Ring 3 GUI ABI, independent client processes and a full compositor (one-shot capture and one resumable client now exist); clipboard, notification system, font/localization and full theme support and remaining native applications (10 trusted kernel tools now exist).
- Physical xHCI platform qualification, USB hubs and generic HID, kernel USB storage, interrupt-driven input, AMD IOMMU and multihop DMAR selection. Build 37 implements a bounded legacy VT-d domain on Q35. General-purpose AHCI/NVMe, APIC/SMP, audio and GPU/compute drivers remain open.
- Build 38 persists one fixed note. Dynamic allocation, broader VFS integration, user file capabilities and persistence; full EXT4 implementation; no writes to internal storage in the Live system.
- Runtime network stack, TCP sockets, system VPN and enforcement of per-app network policy.
- Authenticated pre-boot login, TPM, encrypted root/home, signed boot, installer, recovery.
- In-OS SWP transactions, Linux/Windows ABI compatibility, DEB/RPM/EXE execution, runtimes, CUDA and other compute APIs.

No deadline or fixed number of checkpoints is asserted. Do not tag 0.20.0 as complete until acceptance gates are met or the release scope is explicitly revised.

- Persistent authenticated application identities, policy editor for arbitrary applications, general live revocation and per-user policies; Build 13 adds a graphical editor for bundled applications, Build 15 keys ceilings by EXT4 UUID + inode, but profiles remain volatile. Build 14 cancels the resumable GUI client on policy or mount revision changes; other synchronous apps lack live revocation. Duplicate/cloned EXT4 UUIDs intentionally share policy; no UUID enrollment/repair UI exists.


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
