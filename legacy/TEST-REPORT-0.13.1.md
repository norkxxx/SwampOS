# Farkuad v0.13.1 — Input & boot-menu verification

**Experimental Live preview.** Previous 0.13.0 ISO boot reported by user in VirtualBox; keyboard and mouse did not work. The patch has not been run on a guest or physical PC in this environment.

| Check | Result | Evidence |
|---|---|---|
| `make -j4 all` (kernel, EFI, ISO and lab images) | PASS | `build/logs/farkuad-0.13.1-build.log` |
| PS/2 mocked transactions, ACK/resend, absent/stuck hardware, keyboard/aux demux | PASS | `build/logs/farkuad-0.13.1-input.log` |
| 100,000 mouse decoder bytes, sign/overflow, click and framebuffer bounds (UBSan) | PASS | same log |
| Mock UEFI menu: 100 × 100 ms timeout, arrows, Enter, digits, Esc and motto | PASS | same log |
| Existing desktop arithmetic, 50,000 input events, five views, framebuffer canaries | PASS | same log |
| RAM EXT4 read-back, ISO9660/El Torito/MBR, exact EFI extraction from both FAT volumes | PASS | same log |
| `make -j2 test` full host regression | PASS (exit 0) | `build/logs/farkuad-0.13.1-regression.log` and `.status` |
| Protected SwampBoot GPT image SHA-256 unchanged from previous ISO release | PASS | `8b27dcce28c3a85d1357e9c2708097c6e254ba1162200c2a5b52372a6284da37` |
| USB and optical QEMU guest runs | NOT RUN | `build/logs/farkuad-0.13.1-runtime.log` — QEMU missing |
| VirtualBox input / menu, real PC input | NOT RUN | No VM or physical access |
| Independent security review | NOT RUN | Not performed |

The absent COM1 starvation bug is established from source inspection, not confirmed as the user's sole failure cause. PS/2 host mocks are not hardware emulation. The ten-second menu is nominal (rendering overhead can add time). No USB HID, absolute tablet, Secure Boot or installed-disk decryption added. Historical logs describe earlier releases, not this patch. Prior ISO report: `legacy/TEST-REPORT-0.13.0-iso.md`.
