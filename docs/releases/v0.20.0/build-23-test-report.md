# Build 23 test report

- 4,000 modeled configure/disable lifecycles, 32/64-byte contexts, DCI 31, interval, DMA overlap and failed endpoint state (UBSan).
- `make -j4 all`: PASS (incremental rebuild; not a clean build).
- `make -j4 test`: exit 0.
- `make checksums` and `sha256sum -c SHA256SUMS`: 22/22 PASS.
- Protected SwampBoot firmware and image: byte-identical to Build 22.
- QEMU/VirtualBox unavailable: no guest/physical validation.
- No second clean build.

See `build/logs/build23-*.log` and `.status`.
