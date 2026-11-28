# Build 02 test report

| Gate | Result |
|---|---|
| Clean full build | PASS |
| Full host regression (`make test`) | PASS |
| 100,000 window events and explicit Save/Load hit tests | PASS |
| 50,000 RAMFS data round trips, shell command tests, bounds and clearing | PASS |
| All 20 distribution checksums | PASS |
| QEMU / OVMF / VirtualBox new build | NOT RUN |
| Physical hardware and independent audit | NOT RUN |

One clean build performed for this checkpoint; no new two-clean-build reproducibility claim. Previous interim size-limit failure was resolved through additional size optimization, retaining BIOS geometry. Notes button routing was host-tested; the actual guest interaction remains unverified.

Logs: `build/logs/awakens-build02-clean.log`, `awakens-build02-test.log`, `awakens-build02-checksums.log`. The earlier incremental log is retained.

Locked SwampBoot image SHA-256 matches original Farkuad 0.13.1.
