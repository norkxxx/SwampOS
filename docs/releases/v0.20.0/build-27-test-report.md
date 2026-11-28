# Build 27 test report

- 1,000 full modeled sessions and 20,000 keyboard reports, full/high speed, 32/64-byte contexts, invalid DMA/port rejection, stop and quarantine (UBSan).
- `make -j4 all`: PASS (incremental rebuild; not a clean build).
- `make -j4 test`: exit 0.
- `make checksums` and `sha256sum -c SHA256SUMS`: 22/22 PASS.
- Protected SwampBoot firmware and image: byte-identical to Build 26.
- QEMU/VirtualBox unavailable: no guest/physical validation.
- No second clean build.

See `build/logs/build27-*.log` and `.status`.
