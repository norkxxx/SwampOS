# Feature ledger

## Build 38

Native AHCI and NVMe queues service one explicitly authorized 4 MiB fixture. A bounded writable VFS file uses alternating CRC-checked sectors. QEMU verified cold-boot persistence and previous-generation recovery on both transports. See [the format](docs/PERSISTENCE.md).

## Build 37

| Feature | Evidence |
|---|---|
| Shared xHCI event ring, up to two HID slots | Two QEMU USB devices active together, i8042 disabled |
| Native desktop input | USB typed a shell command, moved the cursor, dragged and resized Notes |
| VT-d-backed buffers | QEMU physical-memory inspection of the active IOMMU tables and revoked root |
| Lifecycle | Guest stop, restart, hot-unplug and refusal profiles recorded in build/logs |

## Build 36

Build 36 keeps a 640 × 360 scene in 900 KiB of kernel memory. Both desktop modes draw there before copying finished pixels to the UEFI framebuffer. Moving the pointer over a static desktop updates the old and new 8 × 13 cursor areas. Dragging a window repaints its old and new bounds in stacking order. Presentation runs at most once every two PIT ticks (about 50 Hz). The input loop processes button and key transitions in order between presentations.

The aggregate host suite and the QEMU/OVMF USB, optical, Classic, recovery and read-only IOMMU profiles passed. The protected boot manager returned locked after invoking SwampLogin. Historical rows below record results from their original checkpoints.

| Feature | Evidence |
|---|---|
| Farkuad baseline | User reported working live desktop/input in VirtualBox; not independently reproduced here |
| Hybrid UEFI ISO | Built and structurally validated |
| Kernel-backed window manager | Compiled; host operations and 100,000 event cycles passed UBSan |
| Classic fallback, ASCII boot menu, 10-second countdown | Retained and mock-tested |
| Native ELF, memory, process groups, EXT4 reader | Existing implementation; host regression passed |
| Five desktop tools | Existing kernel views, now windowed; not isolated GUI apps |
| Secure installed system | Not ready |
| Writable volatile RAMFS | 16 × 4 KiB flat files; kernel shell CRUD and Notes Save/Load; 50,000 data round trips and shell tests passed |
| BIOS kernel growth | 256 KiB payload, eight 32 KiB reads; compiled and static-tested, guest pending |
| Shell command history | 16 volatile entries, PS/2 Up/Down, draft restore; 100,000 key events passed |
| Five additional desktop tools | RAM Files, Hex, Paint, Settings, Guard; compiled and host-tested; kernel-trusted |
| Hierarchical RAMFS | 16 file and 16 directory slots, nested paths and shell operations; host-tested |
| Native GUI capture | Capability 32, syscall 11, 16×8 indexed frame, separate ELF, mock-dispatch host validation |
| USB controller inventory | Four PCI USB controller types; read-only `usb` command |
| USB boot HID codecs | Descriptor subset, keyboard/mouse reports, `usbtest`; no physical transport |
| RTC | Bounded stable CMOS snapshot via `rtc`; mocked host tests, actual I/O pending |
| Expanded RAMFS | 64 × 8 KiB, 64 directories; append, subtree move, bounded offset read and explicit EXT4 import; host-tested |
| ATA PIO transport | LBA28/LBA48 read-only lab driver, 512-byte logical sectors, bounded polling, mock I/O and 50,000 mutations; physical execution pending |
| Explicit resource grants | Selected RAMFS/EXT4 file snapshot, CAP_DATA and optional CAP_CONSOLE, 256 bytes, 50,000 host cycles; guest execution pending |
| Chunked resource reads | Build 11: 64 KiB selected file snapshot, 256-byte transfers; native resource-scan ELF and mock CPU/SDK tests; no new filesystem privileges |
| SwampGuard ceilings | 32 volatile per-volume/per-inode profiles keyed by the full EXT4 UUID + inode; explicit grants checked at every storage ELF launch, audit and host command integration; not persistent |
| SwampGuard editor | Six bundled ELF targets, keyboard/mouse staged ceiling changes, shared audited update backend and stale revision rejection; host-tested, volatile |
| Resumable native GUI | Build 14: one fixed ELF, register/CR3 restore, yield/timer slices, 24-frame Paint demo, policy revision cancellation; 2,000 mock CPU sessions, guest pending |

| Volume-scoped application identity | Build 15: EXT4 UUID extraction, zero-UUID rejection, independent ceilings for equal inode numbers on different UUIDs and same-volume remount stability; cloned UUIDs share policy; host-tested |


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


## Build 31 — Dynamic Live image sizing

- Optical FAT16 and USB FAT32 boot volumes are sized from the actual EFI payload, not fixed 16/64 MiB reservations.
- FAT16 El Torito sector-count and cluster limits are checked; EFI payload is capped at 30 MiB.
- FAT32 sizing enforces at least 65,525 data clusters and includes a 4 MiB reserve.
- Build emits `Farkuad-Live.iso.report` with component sizes, filesystem capacity utilization and ISO SHA-256.
- Host tests verify exact EFI extraction from both volumes and that larger EFI payloads produce larger ISO/raw USB images.


| Runtime xHCI PCI inventory | Build 32: read-only `xhci` command reports BDF, vendor/device, BAR0 width/address and PCI Command Register flags; host-tested, no PCI writes or controller activation |

| UEFI xHCI BAR resource handoff | Build 33: firmware-reported BAR0 aperture via EFI_PCI_IO_PROTOCOL, versioned bootinfo record and live PCI cross-check; host-tested, metadata only |

| xHCI capability probe | Build 34: UC/NX/supervisor-only BAR0 mapping and read-only CAP register validation; physical range alias checks and host tests, hardware execution pending |
| MMIO physical-range protection | Build 34: UEFI handoff retains high RAM/ACPI/loader ranges; refuses MMIO alias and incomplete range maps |

| Intel VT-d inventory | Build 35: ACPI DMAR/DRHD/RMRR/ATSR parsing, xHCI scope coverage report; host-tested, no IOMMU activation |
