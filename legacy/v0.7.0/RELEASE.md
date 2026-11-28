# SwampOS v0.7.0 — SwampBoot | The First Layer Is Trust

![SwampBoot](assets/banner.svg)

SwampBoot introduces an independent x64 UEFI firmware front end, separate locked SwampLogin partition, GPT/FAT32 image generation, read-only GPT and PCI inventory, and an offline signed/encrypted multiboot registry. This release extends Vault without removing the BIOS kernel, scheduler, sandbox prototypes, EXT4 reader or ELF loader.

## Added
- Three relocatable PE32+ EFI artifacts (strict manager, lab manager, separate login).
- Same-disk login discovery and firmware-mediated image loading; fail-closed return.
- Five-partition laboratory GPT image with redundant GPT metadata, FAT32 ESP and login volume.
- Read-only GPT validation and PCI config-space enumeration; shell commands `gpt` and `pci`.
- Offline RSA-4096/AES-GCM encrypted volume provisioning and Ed25519-signed encrypted boot registry, explicit rollback floor.
- Expanded CI gates, mutation testing, docs, SVG assets, ten reproducible artifacts.

## Evidence
Two clean builds produced identical SHA-256 hashes for all ten listed artifacts. Host/mocked tests passed, including a separately resumed final crypto test in the second suite. The BIOS and UEFI guest gates could not run (QEMU/OVMF missing). See TEST-REPORT.md.

## Important limitations
Unsigned lab image; strict policy checks firmware variables but is not an enrolled Secure Boot chain. Login accepts no credentials, unlocks no disk, and has no UEFI kernel handoff. ROOT/HOME are empty in the default GPT image. Offline envelope tools are not transparent block encryption or TPM integration. Existing BIOS demo remains plaintext. No hardware testing or independent audit.

**Pre-release only. Security has layers. The first is trust.**
