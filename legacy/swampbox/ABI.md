# Experimental SwampBox ABI v0

The only current applications are position-independent, trusted build-time test programs copied into a single user code page. No external binaries are accepted. A fresh CR3 is used for each run.

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
| other | unsupported | ignored | ignored | -38 |

Only bit 0 (`CAP_CONSOLE`) exists. A zero grant mask denies console output. No path, network, GPU, USB, raw disk or other device API exists. This is a minimal kernel capability demonstration, not a complete app-policy engine. The timer terminates a busy user test after nominally 100 ticks; user faults terminate the current test, while fatal system faults stop the kernel.
