# Build 15 test report

- `make -j4 all`: exit 0. Kernel, BIOS image, UEFI front ends, GPT lab images, FARKUAD.EFI and hybrid Live ISO rebuilt successfully.
- `make -j4 test`: all emitted host-test recipes passed through the final `tests/vault_host.py`; the overall command was terminated by the execution time limit while that final crypto test was running. `python3 tests/vault_host.py` was then run separately and passed. Do not interpret the timed-out aggregate command as an uninterrupted exit-0 run.
- New `volume-guard-host-test`: PASS under UBSan. Reads actual generated EXT4 images, verifies UUID extraction, rejects a zero UUID, gives two filesystems the same inode number with different UUIDs, checks independent SwampGuard ceilings, remount stability and independent profile removal.
- Updated `guard-host-test`: PASS; 100,000 policy combinations plus real EXT4 storage command dispatch, graphical editor, launch denial, group atomicity, async GUI revocation and remount behavior.
- `build/storage_host` passed with an assertion for the expected EXT4 UUID.
- `make checksums` and `sha256sum -c build/SHA256SUMS`: PASS, 22/22 artifacts.
- `tests/live_image.py`: PASS; ISO9660, El Torito, MBR, FAT16/FAT32 extraction and EFI payload consistency.
- `tests/test_image.py`: PASS; kernel image geometry, ELF segments and unresolved symbol check.
- `tests/git_index_host.py`: PASS; local LFS/index policy fixture. This does not test a remote GitHub push.
- QEMU and VirtualBox are not installed in the build environment. No guest or physical-hardware validation was performed.
- `BOOTX64.EFI`, `BOOTLAB.EFI`, `LOGIN.EFI` and `SwampBoot-GPT.img` are byte-identical to Build 14; the Live EFI payload changed with the kernel and boot-menu checkpoint.
