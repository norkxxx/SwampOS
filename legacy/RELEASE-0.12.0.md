<div align="center">
<img src="assets/banner.svg" alt="SwampOS v0.12.0 Overlord" width="100%" />

# SwampOS v0.12.0 — Overlord

**Security has layers. Tasks have boundaries.**

`EXPERIMENTAL PRE-RELEASE` · `x86_64` · `HOST-TESTED`
</div>

## Major integrated update

- Bounded preemptive user process groups: up to four independent native contexts, timer-driven round-robin, voluntary yield and per-task CPU tick budgets.
- Group-local capability-mediated nonblocking IPC, private writable scratch storage, independent permissions, fault handling and cleanup.
- Native ELF64 now supports a second optional RW/NX segment, initialized data and zero-filled BSS; the old profile stays compatible. Eleven native syscalls and a C demo on EXT4.
- Expanded guest integration gate, scheduler/dispatcher simulation, parser mutation tests, VM tests and Git LFS index preflight.
- Existing SwampBox, Vault, SwampBoot, network lab, SWP, GUI lab and the five target editions retained. Previous release assets and docs included.

See [TEST-REPORT.md](TEST-REPORT.md) for evidence and [OVERLORD](docs/OVERLORD.md) for limitations. Real guest execution remains pending. The separate SwampLogin is still locked and encrypted disk unlock is not integrated. No full desktop, general process manager, native package installation, VPN tunnel, CUDA or foreign execution claimed.

**SWAMP MATRIX // SWAMP OS**  
`YOUR SYSTEM. YOUR RULES. YOUR SWAMP.`
