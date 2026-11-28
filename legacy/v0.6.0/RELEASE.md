# SwampOS v0.6.0 — Vault | Data Has Boundaries

![Vault](assets/banner.svg)

**Experimental pre-release — guest execution remains unverified.**

Vault adds the first disk-backed execution pipeline to the independent SwampOS kernel: a checked block interface, legacy ATA PIO reader, deliberately narrow read-only EXT4 support and a static ELF64 loader for independent native applications. The existing SwampBox capability model is retained with explicit per-run console grants. A separate offline RSA-4096/AES-256-GCM image envelope prototype is included as a stepping stone toward the protected storage design.

## Highlights
- Actual 8 MiB EXT4 fixture generated via libext2fs, with nested directories and a separately built native application.
- Checked disk, inode, directory, extent and ELF bounds; unsupported formats rejected.
- New `storage`, `fscheck`, `ls`, `cat`, `run` and `run-console` commands.
- Host-only offline encryption prototype with passphrase-protected RSA private key and authenticated AES image envelope.
- Two clean builds, 80,000 deterministic parser mutations, existing PMM/VMM and scheduler regression tests, crypto negative tests and checksum comparison.

## Assets
One ZIP contains the complete source, `build/SwampOS.img`, `build/vault.ext4`, `build/hello.elf`, kernel ELF, hashes, test logs, README, local SVG art, release notes and documentation. No keys are distributed.

## Known limitations
No QEMU/physical runtime verification; no general EXT4/VFS, disk write, journal, preemptive user processes, UEFI, signed login partition, block encryption, TPM or LUKS2 implementation. The fixture and boot image are plaintext. The RSA/AES host tool is not a secure boot path. Do not install on real disks or run untrusted binaries.

**Security has layers. Data has boundaries.**
