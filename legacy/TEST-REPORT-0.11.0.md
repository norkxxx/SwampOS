# Integration 0.11.0 — actual local validation

| Check | Result |
|---|---|
| Two clean builds (GCC, Clang, LLD, warnings as errors) | PASS |
| Full host/static regression suite, first and second builds | PASS |
| 17 build artifact hashes identical | PASS |
| Session stress: 100,000 lifecycles per run, bounded copy, scrub, capability checks (UBSan) | PASS |
| Signed SWP store export and byte-identical EXT4 staging image; negative cases | PASS |
| Native integration ELF extracted and validated by kernel storage host test | PASS |
| Git attribute / ignore inspection and Bash script syntax | PASS |
| Locked SwampBoot image compared to previous release | PASS |
| QEMU BIOS and network; OVMF login, kernel and graphics; GPT storage guest | NOT RUN — QEMU absent |
| Git LFS client migration, remote push, CI, PowerShell execution | NOT RUN — tools/remote unavailable |
| Physical hardware and independent security audit | NOT RUN |

The full regression was repeated after the second clean build. New Integration tests additionally ran independently on each build; see raw logs. The stress counts describe host function tests, not guest application runs. QEMU gates were attempted and refused to run without the emulator. No hardware execution is claimed.

## Evidence

Raw logs: `build/logs/`. `SHA256SUMS` verifies the 17 primary artifacts; `IMAGE-SHA256SUMS` is the subset for Git LFS. These checksums are not signatures. Current images are plaintext lab fixtures. The protected login image is unchanged and remains locked.
