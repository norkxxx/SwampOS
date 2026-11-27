# SwampOS v0.2.0 — Onion

**Security has layers.**

![Release](https://img.shields.io/badge/RELEASE-0.2.0_ONION-315c43?style=for-the-badge)
![Status](https://img.shields.io/badge/STAGE-EXPERIMENTAL-526b45?style=for-the-badge)

## What's new
- ELF64 freestanding kernel and x86-64 long-mode transition code.
- Initial three-level page-table setup with 2 MiB supervisor-only pages mapping the first 1 GiB.
- CR0 write-protect enabled; explicit BSS initialization and 64-bit stack.
- Existing serial/VGA shell and all five profile specifications retained.

## Verification
Clean compilation and static image checks passed. QEMU is unavailable in the build environment; boot and shell execution remain unverified.

## Known limitations
BIOS only; assumes long-mode-capable CPU. Initial mappings are writable and executable, not a hardened memory layout. No physical allocator, IDT, scheduler, Ring 3, sandbox, UEFI, encryption or application compatibility yet. RAW image, not ISO. Test only in a disposable VM.

## Assets
- SwampOS-Onion.zip — source and build output
- SwampOS-Onion.img — experimental RAW image

**Your system. Your rules. Your swamp.**
