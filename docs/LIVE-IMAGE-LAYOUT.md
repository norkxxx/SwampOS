# Live image layout — Build 35

The Live builder now sizes the two embedded boot volumes from the exact `FARKUAD.EFI` payload. It no longer reserves fixed 16 MiB and 64 MiB volumes.

## Layout

- ISO9660 starts at sector 0 (2048-byte sectors).
- El Torito catalog is at LBA 22.
- The optical FAT16 image begins at LBA 512 (1 MiB) and is sized for the EFI file, a 512 KiB growth reserve, FAT16 metadata, and ISO-sector alignment.
- The MBR partition entry points to the FAT32 ESP immediately after the optical image.
- FAT32 size follows the payload, a 4 MiB reserve, one-sector clusters, and the FAT32 minimum of 65,525 data clusters. The volume is aligned to 1 MiB.
- Both volumes contain the exact same `EFI/BOOT/BOOTX64.EFI` bytes.

The El Torito catalog stores the optical boot image length in a 16-bit count of 512-byte sectors. Build 35 therefore caps the EFI payload at 30 MiB, leaving room for FAT16 metadata and alignment. A future boot format is required if the EFI application must exceed this limit.

## Current Build 35 report

`build/Farkuad-Live.iso.report` is generated from the actual image and component files. It records kernel binary/ELF, EXT4, EFI, FAT16, FAT32 and total ISO sizes, plus payload utilization and SHA-256.

At Build 35:

| Component | Bytes | MiB |
|---|---:|---:|
| `kernel.bin` | 102,416 | 0.10 |
| `kernel.elf` | 128,800 | 0.12 |
| `vault.ext4` | 8,388,608 | 8.00 |
| `FARKUAD.EFI` | 8,502,272 | 8.11 |
| Optical FAT16 | 9,308,160 | 8.88 |
| USB FAT32 | 34,603,008 | 33.00 |
| Hybrid ISO | 44,959,744 | 42.88 |

The previous fixed image was 84,934,656 bytes (81 MiB). Build 31 removed unused reserved capacity; Build 35 remains payload-sized and future growth in the EFI payload increases the optical volume and, once required, the FAT32 volume and ISO.

## Verification

- `tests/live_image.py` independently extracts both FAT filesystems and verifies that they contain identical EFI payloads.
- `tests/mkiso_dynamic_host.py` builds a second ISO with a 3 MiB larger EFI payload and asserts that the ISO grows.
- `tests/mklive_dynamic_host.py` verifies that the standalone raw USB image and its FAT32 partition grow with the EFI payload.
- `tools/image_report.py` generates the human-readable utilization report.

These are host-side format and payload tests, not UEFI firmware or physical USB boot tests.
