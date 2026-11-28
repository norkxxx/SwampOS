# Build 21 test report

- 4,000 slot enable/address/disable lifecycles, 32/64-byte contexts, DMA overlap and invalid slot/output rejection (UBSan).
- `make -j4 all`: PASS (clean rebuild).
- `make -j4 test`: exit 0.
- `make checksums` and `sha256sum -c SHA256SUMS`: 22/22 PASS.
- Protected SwampBoot firmware and image: byte-identical to Build 20.
- QEMU/VirtualBox unavailable: no guest/physical validation.
- No second clean build.

See `build/logs/build21-*.log` and `.status`.
