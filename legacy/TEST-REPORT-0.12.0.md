# SwampOS 0.12.0 — actual verification report

## Results

| Check | Result | Scope |
|---|---|---|
| Clean builds (GCC, GNU ld, Clang/LLD) | PASS ×2 | All kernel, app, EFI and image targets |
| Full host/static regression | PASS | First final clean build |
| Overlord and Integration host tests | PASS ×2 | Both final clean builds |
| Artifact reproducibility | 18/18 match | SHA-256 lists identical |
| Process groups and scratch | 10,000 + 10,000 per run | FIFO/full/empty/timeout/permissions/scrub; UBSan |
| Actual dispatcher simulations | 1,000 per run | Real C dispatcher; privileged entry and CR3 mocked |
| Two-segment ELF mutations | 20,000 per run | Actual parser, bounds, signed export and EXT4 read-back |
| Page tables / BSS | 1,000 legacy + 1,000 pairs with data | Actual VM C code with simulated RAM; OOM rollback |
| Previous subsystems | PASS | PMM, task assembly, EXT4, GPT, EFI mocks, crypto, hardware, networking models, SWP and GUI |
| Git index check | PASS | Local staged image/attribute/pointer fixture; tested separately after adding target |
| SwampBoot-GPT.img | IDENTICAL | Compared against the supplied 0.11.0 archive |
| Distributed artifact checksum verification | 18/18 OK | Local SHA-256, not signed authenticity |
| BIOS / UEFI / storage / network / graphics guest gates | NOT RUN | All six targets attempted; no QEMU installed |
| Physical hardware / independent security audit | NOT RUN | No environment or auditor |
| Remote LFS transfer / PowerShell | NOT RUN | Git LFS client and PowerShell not present |

Evidence: `build/logs/clean-builds-and-tests.log` (ends COMPLETE), `build/logs/pass1-SHA256SUMS`, `build/logs/git-index-test.log`, `build/logs/runtime-status.log`. Run `make checksums` after rebuilding; the distributed checksums describe this archive only.

The initial helper test using debugfs failed because the tool is missing. It was replaced by a native C host test using SwampOS's own EXT4/ELF parser. Its initial test incorrectly assumed the C ELF entry was exactly USER_CODE rather than an aligned address inside the RX segment; the assertion was corrected to check the entry range. Final tests pass. No claim is made that the guest scheduler, CPU isolation or preboot authentication has been validated on actual hardware.

## Release boundaries

Bounded process groups are synchronous to the trusted shell, not a general process manager. Runtime IPC, actual preemption, guest data/BSS enforcement, and actual boot all need QEMU/OVMF execution. Scratch is volatile, not disk storage. EXT4 remains read-only. Protected SwampLogin remains locked and has no authentication backend; native encrypted block I/O, TPM, Secure Boot enrollment, real VPN/socket enforcement, native package installation, GUI desktop, CUDA, DEB/RPM/EXE execution and SMP remain unfinished. No independent audit or production readiness claim.
