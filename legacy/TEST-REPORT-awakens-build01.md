# Build 01 test report

- `make -j4 all`: PASS after adjusting optimization to fit the 60 KiB BIOS limit.
- `make test`: PASS (full host regression, before version-label/menu-text refresh).
- `make -j4 all checksums` and `make window-host-test live-host-test`: PASS on final checkpoint.
- 100,000 window state events, explicit window lifecycle and input scenarios, framebuffer canaries, UBSan: PASS.
- QEMU/OVMF/VirtualBox runtime and physical PC: NOT RUN (no emulator installed here).
- No independent security audit. No fresh two-clean-build reproducibility claim.

Logs: `build/logs/awakens-build01-*.log`. Earlier logs are historical and not evidence for the current checkpoint.
