# Ecosystem v0.10.0 — local test report

## Build and regression

| Gate | Result | Evidence |
|---|---|---|
| Clean build 1 | PASS | `build/build-pass1.log` |
| Complete host/static regression | PASS | `build/test-pass1.log` (exit 0) |
| Clean build 2 | PASS | `build/build-pass2.log` |
| 16 artifact SHA-256 comparison | PASS | `build/SHA256SUMS-pass1` equals `build/SHA256SUMS` |
| Second ecosystem-only host test | PASS | `build/test-pass2.log` (exit 0) |
| Original locked GPT image | PASS | Byte-identical to Connectivity 0.9.0 image |
| BIOS, UEFI, storage, network and GUI guest gates | NOT RUN | `build/runtime-status.log`; QEMU missing |
| Physical hardware / independent security audit | NOT RUN | Not available |

## New tests (per successful run; executed twice)

- 20,000 deterministic clipped drawing cases, stride/capacity canaries and extreme coordinates under UBSan. Actual host reference image `build/gui-preview.ppm`.
- 100,000 audit appends, ordered ring wraparound, sequence saturation, UBSan.
- 20,000 deterministic ELF/PE/DEB/RPM mutations. 16,374 structurally accepted by the intentionally narrow metadata inspector; acceptance is not safety or execution validation.
- Signed SWP positive and negative tests: wrong key, payload/manifest/signature tamper, path traversal, symlink, extra members, dependency failure/cycle, upgrade, same snapshot rollback/redo, object corruption, explicit permission metadata. Tests generate ephemeral keys, no private keys shipped.
- Native C SDK ELF profile and read-back from EXT4; independent relocatable EFI graphic binary and exact extraction from its FAT32 ESP.

## Retained regression evidence

The complete first pass includes previous E820/PMM (100,000 churn), VM (1,000 cycles), scheduler (1,000 host runs), EXT4/ELF (40,000 mutations), GPT, PCI, ACPI, keyboard, UEFI mock services, registry/provisioning and Vault crypto tests; network transport (50,000 mutations), exact endpoint policy (50,000 checks), virtqueue (150,000 cycles) and VPN decisions/URI metadata tests. See logs for actual individual counts. Old tests were not rerun on pass 2; new ones were.

Host tests execute selected shared code, not guest CPU instruction paths, real device DMA or UEFI firmware. The C SDK demo has not run in Ring 3, the audit hooks have not run in a guest, and the GUI screenshot comparison gate has not run. No production security claims.

## Reproduction

```sh
(cd build && sha256sum -c SHA256SUMS)
make clean
make -j4 all
make test
make checksums
make runtime
make uefi-runtime
make kernel-lab-runtime
make storage-lab-runtime
make network-lab-runtime
make gui-lab-runtime
```
