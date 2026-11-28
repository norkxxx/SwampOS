# Changelog

## [0.6.0] — Vault

### Added
- Checked 512-byte block device abstraction and read-only secondary IDE ATA PIO driver.
- Narrow read-only EXT4 mount, inode/extent validation, directory listing and bounded absolute path traversal.
- External static ELF64 profile and independent native application on a libext2fs-built volume.
- Explicit no-capability and console-capability launch modes using existing isolated-context API.
- Host-only RSA-4096 OAEP / AES-256-GCM image envelope and password-protected PKCS#8 keys.
- Storage parser regression/mutation tests, offline crypto negative tests, expanded runtime CI gate, local release artwork.

### Changed
- Kernel console integrates storage commands and checks the mounted development fixture without auto-executing disk content.
- Build system generates two separate disk images; existing 1 MiB BIOS boot image geometry retained.
- Documentation distinguishes host execution from guest validation and prototype encryption from target full-disk encryption.

### Retained
- SwampBox PMM/VMM, Ring 3 test suite, capability policy, Donkey scheduler, existing shell commands, five edition descriptors and future architecture requirements.

### Pending
- QEMU validation, generalized VFS, EXT4 writes, full process lifecycle, signed preboot login and encrypted block devices.

Earlier history: [SwampBox 0.5.0](legacy/swampbox/CHANGELOG.md), [Donkey 0.4.0](legacy/CHANGELOG-donkey.md).
