# Build 16 test report

- 124,000 command submissions/completions, full-ring backpressure, wrap and cycle changes, invalid completion pointers and error completion (UBSan).
- `make -j4 all`: PASS (incremental rebuild; not a clean build).
- `make -j4 test`: exit 0.
- `make checksums` and `sha256sum -c SHA256SUMS`: 22/22 PASS.
- Protected SwampBoot firmware and image: byte-identical to Build 15.
- QEMU/VirtualBox unavailable: no guest/physical validation.
- No second clean build.

See `build/logs/build16-*.log` and `.status`.
