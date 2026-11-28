# Hardware Awakening — release validation

## Actual host results

Two independent clean builds of the final source were performed. The first completed the entire `make -j4 test` suite. The second completed the new hardware tests and preceding regression tests but was interrupted by the tool execution timeout while compiling the VMM host test; the VMM, storage and vault tests were subsequently run separately and passed. No source or image rebuild occurred between that interruption and the resumed tests. All 13 named artifacts have matching SHA-256 values across the two clean builds. See `build/test-first.log`, `build/test-second.log`, `build/test-second-resume.log`, `build/reproducibility.log`, `build/SHA256SUMS.first` and `build/SHA256SUMS` for evidence. The two clean runs are counted below; these are not guest tests.

| Test | Result |
|---|---|
| Warnings-as-errors GCC/Clang, ELF64 and PE32+ build | PASS |
| Static raw image geometry, payload, ELF segment permissions and undefined symbols | PASS |
| 20,000 PCI configuration mutations; BAR/capability/cycle fixtures | PASS |
| 40,000 ACPI mutations; XSDT/MADT/MCFG checks | PASS |
| 200,000 keyboard scan bytes; modifier and extended-sequence tests | PASS |
| 40,000 EFI memory descriptor mutations; clipping, overlaps, malformed lengths | PASS |
| 100,000 random Ethernet frame inputs + ARP/ICMP/UDP fixtures; output canaries | PASS |
| Actual GPT partition view and EXT4 mount; bad type and range rejection | PASS |
| KERNELLAB PE relocation and exact embedded kernel extraction from FAT32 | PASS |
| 12,000 GPT parser mutations | PASS |
| 28,000 mock EFI manager cases and 40,000 device-path mutations | PASS |
| 2,000 mock SwampLogin calls (locked) | PASS |
| 4,000 PCI enumeration cases | PASS |
| 1,686 signed/encrypted registry byte mutations; wrong keys/password/rollback | PASS |
| Offline encrypted ROOT/HOME provisioning, RSA/AES authentication checks | PASS |
| 2,000 native cooperative scheduler runs | PASS |
| 200,000 PMM churn iterations, pointer and policy tests | PASS |
| 2,000 simulated address-space cycles, page flags, OOM rollback | PASS |
| 80,000 EXT4/ELF mutations | PASS |
| 13 artifact digest comparisons; release manifest verification | PASS |

New host C tests and most prior parsers were built with UBSan; no sanitizer diagnostic was observed in the completed logs. Random frame mutations do not guarantee full protocol coverage. Host CPU, firmware mocks, memory model and file-backed tests do not establish guest execution or hardware enforcement.

## Guest gates

| Gate | Result |
|---|---|
| `make runtime` | NOT RUN — QEMU absent |
| `make uefi-runtime` | NOT RUN — QEMU absent; OVMF absent |
| `make kernel-lab-runtime` | NOT RUN — QEMU absent; OVMF absent |
| `make storage-lab-runtime` | NOT RUN — QEMU absent |
| Physical hardware / independent audit | NOT RUN |

The runtime commands were invoked and failed due to missing prerequisites, as recorded in `build/runtime-status.log`. No runtime logs or screenshots are fabricated. UEFI handoff, PIC/PIT behavior after firmware exit, actual Ring 3, PS/2, ATA and ACPI discovery require guest validation. The locked login has no authentication backend and unlocks nothing. Network tests only operate on memory buffers.

## Reproduction

```sh
./install.sh
make clean
make -j4 test
make checksums
make runtime
make uefi-runtime
make kernel-lab-runtime
make storage-lab-runtime
```
