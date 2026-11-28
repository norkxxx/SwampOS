# Build 25 test report

- 20,000 modeled keyboard/mouse reports, malformed and short reports, pending idle polls, abort and DMA quarantine (UBSan).
- `make -j4 all`: PASS (incremental rebuild; not a clean build).
- `make -j4 test`: exit 0.
- `make checksums` and `sha256sum -c SHA256SUMS`: 22/22 PASS.
- Protected SwampBoot firmware and image: byte-identical to Build 24.
- QEMU/VirtualBox unavailable: no guest/physical validation.
- No second clean build.

See `build/logs/build25-*.log` and `.status`.
