# Farkuad v0.13.0 — Hybrid ISO revision / verification report

**Release class:** experimental Live preview. This is a packaging revision, not a new kernel release. Guest and physical execution remain unverified.

## Executed for this ISO revision

| Check | Result | Evidence |
|---|---|---|
| `scripts/build-live.sh` (kernel/EFI rebuild, Live host tests, ISO, checksum) | PASS | `build/logs/iso-build.log` |
| Live arithmetic (10,000 cases), input (50,000 events), five screens, canaries, UBSan | PASS | same log |
| RAM EXT4 adapter, boundaries and read-back | PASS | same log |
| ISO9660 descriptors, both path tables, directory records, El Torito EFI validation checksum, boot entry, MBR EF extent | PASS | `tests/live_image.py` |
| Exact EFI payload extracted from FAT16 and FAT32, embedded kernel and EXT4 checked | PASS | `tests/live_image.py` |
| Independent ISO reader (`libarchive`) read-back of three files | PASS | `build/logs/iso-libarchive.log` |
| `file` / `blkid` detect ISO9660, MBR, FAT16 and FAT32 | PASS | `build/logs/iso-detection.log` |
| Two ISO generations compared byte-for-byte | PASS | `build/logs/iso-repro.log` |
| All 19 retained build artifacts match original Farkuad archive checksums | PASS | `build/logs/iso-prior-artifacts.log` |
| All 20 current artifact checksums and seven image checksums | PASS | `build/logs/iso-final-verify.log` |
| Flash helper rejects `/dev/null`, shell syntax | PASS | `build/logs/iso-flash-reject.log`; no device written |
| QEMU/OVMF USB boot | NOT RUN | `build/logs/iso-usb-runtime.log` — QEMU missing |
| QEMU/OVMF optical boot | NOT RUN | `build/logs/iso-optical-runtime.log` — QEMU missing |
| Physical USB flashing, boot, input, display | NOT RUN | No physical access |
| Git LFS remote transfer | NOT RUN | Client unavailable |
| Independent security review | NOT RUN | Not performed |

The full prior host regression is documented in [the original Farkuad report](legacy/TEST-REPORT-0.13.0-img.md); it was not rerun in full for this ISO-only revision. Original logs remain in `build/logs/`. The underlying kernel, EFI and EXT4 are byte-identical to the original release. The previous default Live IMG is replaced by the ISO; laboratory images remain. No firmware or real-machine success is inferred from host-side format validation.

## Known limitations

Unsigned x64 UEFI only; no BIOS optical boot, native USB HID, mouse, persistent desktop state, TPM/authenticated installed-disk unlock or production GUI sandbox. Use raw/DD USB flashing on a disposable drive and a spare compatible test machine. Firmware boot compatibility, actual input and framebuffer operation must still be verified. See [Live guide](docs/LIVE.md).
