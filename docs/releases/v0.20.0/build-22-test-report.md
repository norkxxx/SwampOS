# Build 22 test report

- 10,000 USB2 reset cycles, power/debounce, W1C preservation, USB3/PSI rejection, disconnect/timeout/overcurrent (UBSan).
- `make -j4 all`: PASS (incremental rebuild; not a clean build).
- `make -j4 test`: exit 0.
- `make checksums` and `sha256sum -c SHA256SUMS`: 22/22 PASS.
- Protected SwampBoot firmware and image: byte-identical to Build 21.
- QEMU/VirtualBox unavailable: no guest/physical validation.
- No second clean build.

See `build/logs/build22-*.log` and `.status`.
