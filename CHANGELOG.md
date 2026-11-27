# Changelog

## 0.5.0 — SwampBox
### Added
- BIOS E820, PMM, per-context page tables, segment permission split, NX, null/guard pages.
- Ring 3 test contexts, TSS, normalized exception frames, syscall gate, capability policy and timeout.
- Guest tests for syscalls, deny, kernel access, code write, NX stack, I/O, guard, FPU and watchdog.
- Host PMM and VMM stress tests, OOM rollback test, UBSan, strict runtime CI, updated artwork/docs.
### Changed
- Boot success only follows all guest self-tests; image validation now checks ELF segment flags and unresolved symbols.
- SwampShell adds `pmm`, `vmtest`, `sandbox`.
### Retained
- BIOS loader design, console, five profiles, original arena/heap, Donkey cooperative scheduler and architecture targets.
### Known limitations
- QEMU unavailable; Ring 3/interrupt enforcement unverified. No external application loader or full process scheduler.
- No UEFI, encryption, EXT4, driver ecosystem, network, VPN, compatibility or GUI yet.

Previous changelog: [Donkey](legacy/CHANGELOG-donkey.md).
