# Changelog

## v0.8.0 — Hardware Awakening

### Added
- Separate unsigned UEFI KernelLab handoff with embedded kernel, fixed memory reservation, conservative map conversion, new GDT and initial 4 GiB map. Locked login branch unchanged.
- CPU/ACPI/GOP/PCI diagnostic inventory and translated PS/2 keyboard polling.
- PCI controller classification and cycle-safe capability / BAR parsing without device writes.
- Bounded block-slice layer, explicit single read-only GPT/EXT4 mount and independent demo storage disk.
- Offline Ethernet/ARP/IPv4/ICMP/UDP engine and startup self-test.
- Additional host and guest gates, matching OVMF variable templates, hardware map SVG, updated README/release documentation.

### Changed
- BIOS kernel load limit 32 KiB -> 60 KiB; explicit firmware mode entry, UEFI serial-only console.
- Guest QEMU paths disable NICs by default; separate laboratory runtime gates retained in CI.

### Limitations
- Guest CPU/firmware runtime unverified; no native preboot password/TPM or full-disk encryption.
- No APIC routing/SMP, real storage/NIC/USB/GPU drivers, TCP, sockets or actual network transmission. PCI classification and GOP metadata are not hardware support.

---

# Changelog

## 0.7.0 — SwampBoot

- Added separate UEFI strict/lab boot managers and locked login executable.
- Added GPT/FAT32 image and strict redundant GPT reader, PCI enumeration, new console commands.
- Added encrypted offline provisioning, signed/encrypted registry with caller-supplied generation floor.
- Added host/mock/negative tests and OVMF CI gate; ten artifacts reproducible.
- Native login, TPM, Secure Boot enrollment, UEFI handoff and transparent encryption remain pending.

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

## 0.9.0 — Connectivity
- Add experimental opt-in transitional VirtIO Net, split-ring queue handling, bounded polling and shutdown; no default NIC activation.
- Add TCP/IPv4, DNS, DHCP and basic IPv6 validators, plus host mutation coverage.
- Add exact endpoint grants and revocation, VPN route-decision model and offline URI metadata inspector.
- Add isolated QEMU socket integration gate and CI log collection; preserve all previous boot, storage, memory and profile features.
- Update English README, local SVG banner and network diagram, release and test evidence. No guest boot, hardware validation, full VPN or actual application sockets claimed.
