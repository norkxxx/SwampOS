# Build 17 test report

- 10,000 modeled doorbell/completion exchanges, reset/run/stop, timeout, invalid completion, DMA quarantine and unsupported capability rejection (UBSan).
- `make -j4 all`: PASS (incremental rebuild; not a clean build).
- `make -j4 test`: exit 0.
- `make checksums` and `sha256sum -c SHA256SUMS`: 22/22 PASS.
- Protected SwampBoot firmware and image: byte-identical to Build 16.
- QEMU/VirtualBox unavailable: no guest/physical validation.
- No second clean build.

See `build/logs/build17-*.log` and `.status`.
