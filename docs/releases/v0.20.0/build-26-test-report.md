# Build 26 test report

- 4,000 full-speed MPS probes, 8/16/32/64 bytes, both context sizes, invalid size and failed context update (UBSan).
- `make -j4 all`: PASS (clean rebuild).
- `make -j4 test`: exit 0.
- `make checksums` and `sha256sum -c SHA256SUMS`: 22/22 PASS.
- Protected SwampBoot firmware and image: byte-identical to Build 25.
- QEMU/VirtualBox unavailable: no guest/physical validation.
- No second clean build.

See `build/logs/build26-*.log` and `.status`.
