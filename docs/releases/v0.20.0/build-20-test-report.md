# Build 20 test report

- Integrated enumeration of two HID interfaces, 10,000 read transactions, OUT and short IN, fault/timeout and DMA retention on failed halt (UBSan).
- `make -j4 all`: PASS (incremental rebuild; not a clean build).
- `make -j4 test`: exit 0.
- `make checksums` and `sha256sum -c SHA256SUMS`: 22/22 PASS.
- Protected SwampBoot firmware and image: byte-identical to Build 19.
- QEMU/VirtualBox unavailable: no guest/physical validation.
- No second clean build.

See `build/logs/build20-*.log` and `.status`.
