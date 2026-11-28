# Experimental SwampBox ABI v0

Trusted build-time test programs and constrained native ELF executables are copied into a single user code page. A fresh CR3 is used for each run.

| Virtual range | Permission | Purpose |
|---|---|---|
| `0x00001000`–`0x07ffffff` | supervisor only | low physical identity map; text RX, rodata R/NX, other pages RW/NX |
| `0x40000000`–`0x40000fff` | user RX | code and constant data |
| `0x40001000`–`0x40001fff` | absent | guard |
| `0x40002000`–`0x40002fff` | user RW/NX | stack |

Initial RSP is `0x40003000`, RFLAGS `0x202` (IOPL 0); GPRs are cleared. TSS supplies a kernel transition stack. No FPU context support: CR0.TS is set. Ring 3 is entered via IRETQ, syscalls use interrupt gate `0x80`.

| RAX | Operation | RDI | RSI | Returns |
|---:|---|---|---|---|
| 0 | exit | status (low byte) | ignored | does not return |
| 1 | console write | user pointer | 1–256 bytes | byte count, -1 (denied), -14 (bad address) |
| 2 | get caps | ignored | ignored | granted mask |
| 3 | run ID | ignored | ignored | nonzero run ID |
| 4 | assigned resource read | destination | 1–256 bytes | byte count / EOF / negative errno; RDX = offset |
| other | unsupported | ignored | ignored | -38 |

Bit 0 (`CAP_CONSOLE`) and bit 1 (`CAP_DATA`) exist. A zero grant mask denies console output. No path, network, GPU, USB, raw disk or other device API exists. This is a minimal kernel capability demonstration, not a complete app-policy engine. The timer terminates a busy user test after nominally 100 ticks; user faults terminate the current test, while fatal system faults stop the kernel.

## Vault external executable ABI
External ELF64 follows the constrained profile in STORAGE.md; syscall numbers and register convention are unchanged. `run` has zero grants, `run-console` grants CAP_CONSOLE. At Vault release there were no new syscalls or persistent process IDs.

## Ecosystem C SDK
See [SDK.md](SDK.md). The inline C interface originally used exactly three syscall numbers. The standalone `/bin/sdk-demo` is built from C and included in the public demo image. No additional syscall privileges are granted.

## Integration ABI extension
`run-data` grants both CAP_CONSOLE and CAP_DATA and snapshots only `/welcome.txt` (max 256 bytes). RAX=4 accepts only a user stack destination, RSI length, RDX byte offset. EOF returns zero. Absent grant returns -1; invalid writable range returns -14. RAX=3 returns a per-run nonzero ID. No simultaneous user tasks, arbitrary path access, file writes or IPC. See INTEGRATION.md.

## Overlord extension (0.12.0)

The preceding sections describe historical releases. The new optional RW/NX page at `0x40004000` supports initialized data and zero-filled BSS; `0x40003000` remains unmapped. User buffer checks accept this page only for contexts with a valid data mapping, and still enforce 256-byte bounds. Existing stack/code checks remain. Group members have separate CR3 roots; up to four are scheduled on one CPU.

| RAX | Operation | RDI | RSI | RDX |
|---:|---|---|---|---|
| 5 | yield | ignored | ignored | ignored |
| 6 | IPC send | source | 1–64 bytes | destination group slot |
| 7 | IPC receive | destination | 1–64 bytes | ignored |
| 8 | group slot | ignored | ignored | ignored |
| 9 | scratch write | source | 1–256 bytes | offset |
| 10 | scratch read | destination | 1–256 bytes | offset |

CAP_IPC = bit 2; CAP_SCRATCH = bit 3. IPC is group-only, nonblocking and requires both sender and receiver grants. `yield` is a no-op in a single run, and its slot is zero. CAP_DATA snapshots remain single-run only. See [OVERLORD](OVERLORD.md) for all return codes and constraints.

## Awakens Build 06 extension

Syscall 11: `gui_publish(address, length)`; capability bit 5 (32), bit 4 remains reserved. Length exactly 128; source must be readable and entirely mapped. Palette indices 0–5. Return 128 on staging success; -1 denied, -22 invalid format/size, -14 inaccessible range. Single capture session only. See [GUI client](GUI-CLIENT.md).

## Awakens Build 11 resource extension

Explicit `run-resource` grants now allow a read-only snapshot of up to 65,536 bytes. Syscall 4 remains bounded to 1–256 bytes per invocation; `RDX` is a byte offset. EOF returns zero even for out-of-range offsets. The legacy `run-data` path retains its 256-byte limit. SDK constants `SWAMP_DATA_MAX` and `SWAMP_DATA_READ_MAX` document the new limits.
