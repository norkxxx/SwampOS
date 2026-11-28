# SwampBoot 0.7.0 — test evidence

## Executed locally

Two clean `make -j2 test` builds; first completed as a single invocation, second was interrupted by the execution-tool timeout during the final `vault_host.py` test. That test was rerun separately without rebuilding and passed. The second log records the resumption explicitly. An earlier full developmental test also passed, not counted below.

| Check | Result across two final builds |
|---|---|
| GCC / Clang / LLD with warnings as errors | PASS |
| BIOS boot image and ELF static checks | PASS |
| GPT parser / mutations under UBSan | PASS: 12,000 total, 8,720 rejected, 3,280 allowed header-padding changes outside CRC |
| Strict and lab EFI manager mock | PASS: 28,000 cases |
| EFI device-path mutation tests | PASS: 40,000 calls |
| Separate login mock | PASS: 2,000 locked returns |
| Independent GPT/FAT32/PE extraction | PASS |
| PCI mock | PASS: 2,000 full + 2,000 early-stop scans |
| Signed/encrypted registry | PASS: 1,686 byte mutations, wrong key/password, floor, truncation, invalid paths |
| Offline encrypted ROOT/HOME provisioning | PASS, no overwrite and no plaintext input accepted by shape check |
| Scheduler / PMM / VMM | PASS: 2,000 / 200,000 / 2,000 |
| EXT4/ELF mutations | PASS: 80,000 |
| RSA/AES offline crypto | PASS: round trip, wrong password/key, tampering, truncation, no unauthenticated output |
| 10 build artifact SHA-256 comparisons | MATCH |
| BIOS QEMU runtime | NOT RUN: QEMU missing |
| UEFI OVMF runtime | NOT RUN: QEMU/OVMF missing |
| Physical hardware / audit | NOT RUN |

Test counts describe executed cases, not coverage percentages or a proof of security. EFI mock tests exercise manager C logic against simulated services, not native firmware. Mutation tests are deterministic, not exhaustive fuzzing. No firmware or kernel runtime claims are made. Actual logs and checksums are under `build/`.
