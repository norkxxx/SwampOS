# SwampOS v0.5.0 — SwampBox | Boundaries Have Layers

**Experimental pre-release · x86_64 · BIOS · C / Assembly**

SwampBox is the largest kernel architecture change since Onion: a firmware-aware physical frame allocator, fine-grained paging, distinct address spaces, ring transitions and a minimal default-deny syscall policy. The Donkey scheduler and earlier console capabilities remain.

## Highlights
- BIOS E820 map, reserved-range filtering, 4 KiB frames, 128 MiB managed-address ceiling.
- RX kernel text, R/NX constants, RW/NX data; separate user roots, RX code, NX stack and unmapped guard.
- GDT/TSS, IRETQ entry, interrupt `0x80`, console grants, bounded pointers, fault cleanup and timer watchdog.
- Nine built-in guest checks; startup fails closed if a check fails.
- Host stress tests, UBSan, strict QEMU CI gate, new local green SVG artwork and comprehensive English documentation.

## Validation
Host/static tests pass; QEMU not installed in the packaging environment. Guest enforcement and physical boot remain **unverified**. Do not treat this build as a production sandbox. See TEST-REPORT.md.

## Assets
One complete ZIP: source, raw image, ELF, hashes, local SVGs, README, changelog, release notes, profiles, docs and tests. Run `make test` and `make runtime` before relying on the image.

**Security has layers. Now they have boundaries.**
