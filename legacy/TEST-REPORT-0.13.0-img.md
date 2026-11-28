# Farkuad 0.13.0 / verification report

**Release class:** experimental Live preview; no guest or physical hardware claim.

## Executed locally

| Check | Result | Evidence |
|---|---|---|
| Clean `make -j4 all`, two runs | PASS | `build/logs/build1.log`, `build2.log` |
| Full host regression (`make test`) | PASS | `build/logs/test1.log` |
| Repeated Live tests on second build | PASS | `build/logs/test2.log` |
| 20 artifact SHA-256 reproducibility | PASS | `build/logs/SHA256SUMS-first`, `build/SHA256SUMS`, empty `repro.log` (diff exit 0) |
| GOP layout validation, 10,000 calculator cases, 50,000 input events, five rendered views, buffer canaries | PASS | Each Live host test; UBSan enabled |
| Framebuffer page-table construction, supervisor/NX mapping, shared user roots, partial allocation rollback | PASS | VM host regression; simulated physical memory, not privileged execution |
| Embedded RAM EXT4 adapter, file read-back, bounds and firmware overlap rejection | PASS | `ramdisk_host` |
| MBR type EF, both FATs, fallback path, exact EFI/kernel/EXT4 extraction | PASS | `tests/live_image.py`; static |
| `scripts/build-live.sh` | PASS | `build/logs/build-live-script.log` |
| Linux flash helper shell syntax and refusal of `/dev/null` | PASS | `build/logs/flash-reject.log`; no device written |
| Locked SwampBoot image SHA-256 compared with 0.12.0 archive | IDENTICAL | `8b27dcce28c3a85d1357e9c2708097c6e254ba1162200c2a5b52372a6284da37` |
| Distributed artifact checksum verification | PASS | `build/logs/final-verify.log` |
| QEMU/OVMF USB runtime | NOT RUN | `build/logs/live-runtime.log`: QEMU missing |
| Physical USB flashing, boot, input, display | NOT RUN | No physical access |
| Git LFS network push | NOT RUN | No LFS executable / remote validation |
| Independent security review | NOT RUN | Not performed |

The local renderer generated `assets/farkuad-preview.png` from the actual C drawing code with example console contents. It is not a guest screenshot. The kernel framebuffer map, real UEFI exit and USB handoff are not runtime-validated. The GUI's actual file preview depends on the same EXT4 reader verified by host storage tests; whole-guest UI integration is covered only by the pending runtime gate. The previous regression suite covers PMM, VMM, parsers, scheduling models, package transactions and cryptographic host prototypes, not production isolation.

Two clean builds produced matching SHA-256 values for all 20 listed artifacts. The full regression ran on build 1; new Live host tests were repeated on build 2 and once more by the build script. The guest gate is implemented and wired into CI. Installation of missing emulator dependencies was attempted but package index access timed out (see `apt-attempt.log`).

## Known limitations

Unsigned Live-only route; no native USB HID, no mouse, no persistent desktop state, no installed system disk encryption, no TPM or actual SwampLogin authentication, no complete GUI sandbox, no real VPN or foreign executable runtime. The desktop tools execute as trusted kernel code. Firmware fixed-address allocation, legacy interrupt/timer availability, supported GOP format and PS/2-compatible input are hardware constraints. Use a disposable USB and spare PC only after guest validation if possible.
