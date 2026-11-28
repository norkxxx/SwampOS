# Awakens development series

Target release: 0.20.0. Internal checkpoints are not public releases. The original Farkuad 0.13.1 ZIP remains the user-confirmed fallback. Never label an unexecuted guest test as passed.

## Build 01 — window manager

`desktop/window.c` adds a bounded five-window stack, focus, z-order, drag/resize, minimize/maximize/restore, close, launch menu and taskbar. The desktop remains in Ring 0: these are windowed views of existing kernel tools, not isolated GUI processes. Tab opens the next tool; Escape focuses Terminal. The first boot menu option enables the window manager; the second retains the classic keyboard-only view. COM1 recovery and firmware return are unchanged.

The logical work area is 640×330 plus a 30-pixel taskbar, scaled 2× at 1280×720 or larger. Rendering is clipped to the framebuffer, with per-window text truncation. No hardware acceleration or dirty-rectangle compositor. The legacy BIOS 60 KiB kernel limit is retained using size optimization of selected translation units.

## Acceptance gates

- Build 01: host window operations, fuzzed event model and framebuffer canaries; guest test still required.
- Remaining work: writable session storage, GUI clients in Ring 3, input and hardware drivers, runtime network and storage, protected login, signed native packages, compatibility and final guest/hardware validation.

## Build 02 — writable volatile files

`filesystem/ramfs` implements an independent flat in-memory file namespace: 16 entries, at most 4,096 bytes per entry, 31-character names, bounded binary reads and writes, enumeration, copy, rename and removal. Names cannot contain separators or begin with a dot. Copy/rename reject an existing destination; write replaces an existing file after validating the length. Removed entries and truncated tails are cleared. No locking is required in the current serialized kernel shell/desktop path; this is not yet a multi-user VFS or a syscall-accessible namespace.

Notes Save/Load writes/reads `notes.txt` in the same RAM namespace; the editor remains limited to 512 characters. Use `ramls`, `ramstat`, `ramwrite NAME TEXT`, `ramcat NAME`, `ramcp SOURCE DEST`, `rammv SOURCE DEST` and `ramrm NAME`. Reboot discards every file. The original embedded EXT4 and physical disk restrictions are unchanged.

## Build 03 — kernel growth and terminal history

The legacy BIOS lab now requires EDD packet-read support and at least 512 KiB conventional memory. It reads eight contiguous 32 KiB chunks into 0x10000–0x4ffff, avoids crossing physical 64 KiB boundaries, then copies 256 KiB to 0x100000. The stack at 0x70000 is outside that region. `boot/layout.inc` supplies assembler and Python image constants. The old 60 KiB ceiling is removed for BIOS only; the UEFI loader and hybrid ISO are independently built. Actual BIOS execution remains untested.

The native PS/2 decoder recognizes extended Up/Down make codes. The shell maintains a bounded 16-command in-memory history and restores an unfinished draft when navigating down. The line remains limited to 127 characters. Recalling a command emits a new prompt/line; this is not an ANSI terminal with cursor-addressed editing. No history is persisted. GUI clients other than Terminal do not interpret those keys as shell history.

## Build 04 — desktop utilities

See [Desktop tools](DESKTOP-TOOLS.md). Ten window IDs, five original views, five new kernel-trusted utility clients. Guest validation pending.

## Build 05 — RAMFS directory hierarchy

See [RAMFS hierarchy](RAMFS-TREE.md) for path grammar, limits and commands. Volatile only.

## Build 06 — first native GUI capture

See [GUI client ABI](GUI-CLIENT.md). One-shot capture, capability 32; no direct framebuffer access or asynchronous event loop.

## Build 07 — USB groundwork and RTC

See [USB and RTC](USB-RTC.md). Read-only inventory and boot HID decoding, not a physical USB driver. RTC uses legacy CMOS port reads; hardware verification pending.

## Build 08 — expanded volatile filesystem

See [RAMFS volume](RAMFS-VOLUME.md). 64 × 8 KiB and 64 directories, append, range reads and subtree moves; explicit import from mounted read-only EXT4. No persistence or disk writes.

## Build 09 — ATA transport

See [ATA PIO](ATA-PIO.md). LBA48 read-only legacy IDE, no AHCI/NVMe or guest validation. Live driver entry remains blocked.

## Build 10 — Explicit application resource grants

See [Resource grants](RESOURCE-GRANTS.md). One selected file snapshot, maximum 256 bytes; no persistent policy or arbitrary file access. Guest validation pending.

## Build 11 — Chunked native file resources

64 KiB selected snapshot, unchanged 256-byte syscall transfer ceiling, native `/bin/resource-scan` utility, static staging to preserve stack budget and full scrub on return. See RESOURCE-GRANTS.md.

## Build 12 — SwampGuard ceilings

See [Guard ceilings](GUARD-CEILINGS.md). All storage ELF launch paths enforce a volatile per-volume/per-inode ceiling. Host tests use real EXT4 with mocked CPU execution.

## Build 13 — SwampGuard graphical editor

See [Guard editor](GUARD-EDITOR.md). Shared audited ceiling updates, staged edits, key and mouse controls and stale-policy rejection. Six bundled native ELF targets; no new grant mechanism.

## Build 14 — Resumable native GUI

Single fixed Ring 3 client, two-tick quantum, cooperative yield, bounded session and revision-checked cancellation. See [GUI-ASYNC.md](GUI-ASYNC.md). Actual CPU resume remains unverified.

## Build 15 — volume-scoped SwampGuard identity

See [Guard identity](GUARD-IDENTITY.md). The strict EXT4 reader extracts and validates the 128-bit filesystem UUID. Launch ceilings are keyed by UUID plus inode, so equal inode numbers on different UUID volumes do not collide. The same volume retains its profile across unmount/remount during one boot. Duplicate UUID clones share policy; profiles remain volatile and unauthenticated.


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
