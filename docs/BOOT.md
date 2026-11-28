# SwampBoot 0.7.0 — UEFI and GPT

## Current execution boundaries

The existing BIOS kernel and two-disk ATA/EXT4 demo still build independently. The new firmware front end is a distinct, freestanding PE32+ application chain; **it does not load the kernel**. The production-policy build `BOOTX64.EFI` refuses to continue unless firmware reports `SecureBoot=1` and `SetupMode=0`; it is unsigned, not provisioned with a trusted key, and is not a complete Secure Boot implementation. `BOOTLAB.EFI` explicitly permits non-secure firmware and is what the laboratory GPT image embeds as `EFI/BOOT/BOOTX64.EFI`. It must not be signed as a trusted production loader.

The manager uses UEFI Loaded Image, Device Path and Simple File System protocols to find exactly one matching laboratory login partition on the same hardware-device path as its own image. It constructs `\\EFI\\SWAMP\\LOGIN.EFI` and calls firmware LoadImage/StartImage; any load failure or login return leaves the manager in a failed/locked state. It relies on the firmware's device path integrity and Secure Boot policy. The static laboratory GUID is not authenticated identity or a unique installation identifier. No OS handoff is possible even if a child returns success. The separate login app prints locked status and deliberately does not collect credentials. It has no cryptographic key backend, TPM, encrypted-block driver or user authentication. No UEFI runtime or physical test was completed locally.

## Layout

| GPT name | MiB | Content |
|---|---:|---|
| SWAMPBOOT | 64 | FAT32 ESP, lab boot manager |
| SWAMPLOGIN | 64 | FAT32 basic-data partition, login application |
| SWAMPROOT | 17 | Empty by default, optional offline envelope |
| SWAMPHOME | 17 | Empty by default, optional offline envelope |
| SWAMPRECOVERY | 2 | Reserved, empty |

Protective MBR, 1 MiB alignment, primary and backup GPT headers and entry arrays with CRC32; fixed laboratory disk/partition UUIDs. The default image is 166 MiB including alignment. The GPT parser accepts a constrained 512-byte-sector/128-entry subset; both tables and header CRCs must match, duplicate IDs and overlapping ranges are rejected. CRC32 detects corruption, not malicious tampering. Kernel shell `gpt` inspects the secondary IDE disk, and `pci` reads legacy CF8/CFC config space without modifying device registers. No PCIe ECAM, DMA, IOMMU or driver binding yet.

## Offline registry

`tools/boot_registry.py` uses an external Ed25519 trust anchor to sign the encrypted RSA-OAEP/AES-GCM envelope and verifies its signature before decryption. Schema restricts entries to bounded IDs, partition GUIDs, EFI paths and SHA-256 metadata; an explicit trusted minimum generation rejects older signed records. The registry does not verify a loader file, store a trusted counter, or drive firmware chainloading. The supplied floor must come from a trusted source; the generation is visible metadata. No TPM counter, platform key enrollment or signed recovery is implemented. No private keys are distributed.

## Next implementation gates

Native reviewed crypto backend and password KDF; signed firmware image enrollment/revocation; signed and authenticated boot policy with a TPM-sealed rollback counter; encrypted block device and actual pre-boot ROOT/HOME unlock; UEFI memory map/ExitBootServices and kernel handoff; framebuffer/ACPI; hardware validation. Existing BIOS demo remains plaintext and is not part of the future trusted boot chain.

## 0.8.0 / Independent KernelLab handoff

The preceding 0.7 description remains accurate **for the locked boot manager and SwampLogin**. A separate disposable GPT image (`KernelLab-GPT.img`) now embeds `KERNELLAB.EFI` as the firmware fallback. This is unsigned, refuses unless firmware reports SecureBoot disabled, and contains only a plaintext lab kernel; ROOT/HOME are empty. It is not referenced or loaded by either manager or SwampLogin. Never enroll it as trusted firmware code. No authorization bypass is introduced into the locked chain.

The lab requests a fixed 1–4 MiB memory reservation, copies the raw linked kernel at 1 MiB, records GOP geometry/ACPI RSDP, builds a temporary GDT and a four-level 4 GiB identity map, converts the final EFI memory map (only conventional, non-runtime RAM in 4–128 MiB, rejects overlaps), and retries `ExitBootServices` on a stale key. It transfers with interrupts disabled, a fresh stack, CR0.WP, no global TLB mappings, and SMAP off until mediated user copies exist. The kernel then constructs its own 4 KiB page tables, excludes the lab reservation from allocation, and uses serial console (no direct VGA write in UEFI mode). GOP metadata is diagnostic only; there is no framebuffer driver yet. The code rejects LA57, excessive memory descriptors, missing low allocation, or a trampoline above the initial identity range. Firmware/runtime/APIC state assumptions have not been verified on OVMF. Neither a successful PE build nor a mock memory-map test proves guest execution.

BIOS loading now reads 120 sectors (60 KiB) and explicitly clears the UEFI entry magic. This remains an independent plaintext lab path. `make kernel-lab-runtime` is a distinct OVMF gate; `make uefi-runtime` continues to require the original login-locked markers. `make storage-lab-runtime` tests the independent GPT/EXT4 demo. Each fails rather than claiming success when prerequisites are missing.

## Awakens Build 03 — extended legacy payload

The independent legacy BIOS lab now reserves 256 KiB for the kernel image (eight 64-sector reads, 32 KiB per read). `boot/layout.inc` is the shared source of layout constants. EDD and conventional-memory checks precede the reads. The linked boot sector and layout passed static tests; actual firmware runtime is still pending. UEFI Live continues to use its independent boot chain.
