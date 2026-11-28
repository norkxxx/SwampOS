# Build 29 test report

- PCI resource validation and 10,000 bounded MMIO operations, UC/NX/supervisor PTE checks, physical alias and OOM rollback (UBSan).
- `make -j4 all`: PASS (incremental rebuild; not a clean build).
- `make -j4 test`: exit 0.
- `make checksums` and `sha256sum -c SHA256SUMS`: 22/22 PASS.
- Protected SwampBoot firmware and image: byte-identical to Build 28.
- QEMU/VirtualBox unavailable: no guest/physical validation.
- No second clean build.

See `build/logs/build29-*.log` and `.status`.
