# Build 19 test report

- 60,000 control transactions, setup/data/status, ring wrap, short IN packets, DMA bounds and poisoned endpoints (UBSan).
- `make -j4 all`: PASS (incremental rebuild; not a clean build).
- `make -j4 test`: exit 0.
- `make checksums` and `sha256sum -c SHA256SUMS`: 22/22 PASS.
- Protected SwampBoot firmware and image: byte-identical to Build 18.
- QEMU/VirtualBox unavailable: no guest/physical validation.
- No second clean build.

See `build/logs/build19-*.log` and `.status`.
