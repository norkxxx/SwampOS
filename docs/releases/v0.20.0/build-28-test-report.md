# Build 28 test report

- 10,000 DMA lease cycles, stale-token rejection, pinning, halt gate, fragmentation, table exhaustion and supervisor-only mapping (UBSan).
- `make -j4 all`: PASS (incremental rebuild; not a clean build).
- `make -j4 test`: exit 0.
- `make checksums` and `sha256sum -c SHA256SUMS`: 22/22 PASS.
- Protected SwampBoot firmware and image: byte-identical to Build 27.
- QEMU/VirtualBox unavailable: no guest/physical validation.
- No second clean build.

See `build/logs/build28-*.log` and `.status`.
