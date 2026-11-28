# Connectivity v0.9.0 — Test evidence

## Executed locally

| Gate | Result | Evidence |
|---|---|---|
| Clean build 1 | PASS | `build/build-pass1.log` |
| Full host/static regression | PASS | `build/test-pass1.log` |
| Clean build 2 | PASS | `build/build-pass2.log` |
| Connectivity suite, second run | PASS | `build/test-pass2.log` |
| 13 artifact SHA-256 comparison | PASS | `build/SHA256SUMS.pass1`, `build/SHA256SUMS` |
| BIOS guest | NOT RUN | QEMU missing |
| Locked UEFI guest | NOT RUN | QEMU missing |
| UEFI kernel lab guest | NOT RUN | QEMU missing |
| Storage lab guest | NOT RUN | QEMU missing |
| VirtIO network guest | NOT RUN | QEMU missing |
| Physical hardware / independent audit | NOT RUN | Outside local execution environment |

## New suite (per run)

- 50,000 deterministic mutated/truncated protocol fixtures: TCP/IPv4, DNS, DHCP and IPv6. Positive and negative fixtures included.
- 50,000 randomized app identity permission checks, exact endpoint match, grant capacity and revocation.
- 150,000 split-ring submit/complete cycles, 16-bit index wraparound, duplicate/invalid completions and excessive pending count. These are CPU host simulations, not device DMA tests.
- 32 route policy combinations, no fallback on required tunnel failure.
- 10,000 URI mutations, secret-field omission and malformed/insecure flag rejection (offline metadata only).

C connectivity suite uses GCC with `-Wall -Wextra -Werror -fsanitize=undefined -fno-sanitize-recover=all`. The production kernel is freestanding and compiled with warnings as errors; UEFI uses Clang and LLD. The complete legacy regression suite was executed once on the final code; only the new suite was repeated on the second clean build. Both final build manifests matched exactly. Prior intermediate builds are not counted in these totals.

## Limits

The host tests do not establish guest execution, actual DMA correctness, physical hardware support, real TCP sessions, an enforced application firewall, VPN encryption or a deployed kill switch. QEMU/OVMF gates must run in a suitable environment. `build/runtime-status.log` contains the real failed prerequisite attempts and exit codes. There are no synthetic guest logs. The signed/pre-boot key release and native encrypted block I/O remain unimplemented; no production security claim.
