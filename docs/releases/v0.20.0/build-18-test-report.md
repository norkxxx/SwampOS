# Build 18 test report

- 50,000 descriptor mutations, exact control sequence, short transfers, fail-stop, header consistency and boot-only composite policy (UBSan).
- `make -j4 all`: PASS (incremental rebuild; not a clean build).
- `make -j4 test`: exit 0.
- `make checksums` and `sha256sum -c SHA256SUMS`: 22/22 PASS.
- Protected SwampBoot firmware and image: byte-identical to Build 17.
- QEMU/VirtualBox unavailable: no guest/physical validation.
- No second clean build.

See `build/logs/build18-*.log` and `.status`.
