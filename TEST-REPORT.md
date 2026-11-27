# SwampBox v0.5.0 — Test report

## Actual execution results

| Gate | Result |
|---|---|
| Clean freestanding builds | PASS (2) |
| Warnings as errors | PASS |
| ELF64 entry, RX/R/RW segments, symbols, raw image | PASS (2) |
| Native cooperative scheduler | PASS (2 × 1,000 runs) |
| PMM exhaustion / reserved and overlap handling | PASS (2 runs) |
| PMM churn + policy / pointer checks (UBSan) | PASS (2 × 100,000 churn iterations) |
| Host VM table construction, OOM rollback (UBSan) | PASS (2 × 1,000 address-space cycles) |
| Boot/kernel/image binary reproducibility | PASS (SHA-256 identical across both clean builds) |
| QEMU integration | NOT RUN (executable absent; make runtime exited 2) |
| Physical machine | NOT RUN |

The host VM simulation uses the same table-building code with a host RAM array. It validates flags and allocation behavior, not hardware enforcement. Native scheduler testing does not validate BIOS boot or ring transitions. The nine guest checks exist in source but were not executed here.

Evidence: `build/test-first.log`, `build/test.log`, `build/reproducibility-first.txt`, `build/reproducibility-second.txt`, `build/runtime-status.log`, `build/SHA256SUMS`.

Build host: 14.2.0; GNU ld (GNU Binutils for Debian) 2.44; x86_64. No production security or compatibility claims.
