# Vault 0.6.0: storage and native executable profile

The boot disk remains the existing 1 MiB BIOS RAW image. A separate 8 MiB data disk (`build/vault.ext4`) is attached as secondary IDE master, using a disposable QEMU snapshot overlay. This is a deliberately constrained development fixture generated with upstream libext2fs, not an installer or encrypted root disk. The kernel implements its own block bounds, ATA PIO LBA28 IDENTIFY/read path, EXT4 inode/extent/dirent parsing, absolute path lookup, and ELF validation. The storage code has no write entry point. ATA guest execution is not locally validated.

## Supported EXT4 subset
- One block group, 4 KiB blocks, 128/256-byte inodes, 32-bit block counts and descriptors, clean state, FILETYPE + EXTENTS incompatibility bits. No compat bits; only sparse-super and large-file RO-compat bits accepted. The generated fixture has neither RO-compat bit.
- Regular files and directories only; inline extent root of depth zero, at most four extents, contiguous logical coverage, no holes/unwritten extents, file limit 64 KiB, path depth 8, component limit 255 bytes. Rejects dot traversal, duplicate matching names, unsupported inode flags, invalid lengths, out-of-range block references and unsupported features.
- No journal replay, metadata checksums, 64-bit, flex_bg, htree, indirect extents, symlinks, write support, permissions enforcement, bitmap ownership verification or global consistency validation. Not a general EXT4 mount; malformed images may be rejected but successful parsing is not proof of integrity. The fixture is plaintext and intended for trusted development only.
- When the supported disk is mounted, boot checks the known test fixture (pattern file and external ELF parse). No executable from disk is run automatically. An incompatible or missing disk leaves storage unavailable; a mounted but invalid fixture fails the development boot gate. No VFS mount table or user file-descriptor syscall exists yet.

## Native ELF subset
ELF64 little-endian x86_64 ET_EXEC, one PT_LOAD exactly RX, fixed VA/PA 0x40000000, page-aligned file offset, at most 4096 bytes, p_filesz == p_memsz, entry inside segment, no dynamic loader or writable data segments. Existing sandbox creates a fresh RX code page, RW/NX stack, unmapped guard and supervisor-only kernel branch. No argument vector, relocations, libc, concurrent processes, FPU, or filesystem syscalls. `run /bin/hello` grants no capabilities; `run-console /bin/hello` explicitly grants console. These are shell operator controls, not a persistent policy database. No signature checking or process identity verification yet.

## Runtime integration
`make runtime` requires QEMU; checks boot, storage, listing, reading, and both external-app grant states, in addition to earlier CPL3 checks. The host tests exercise the real parser code against generated disk bytes with a memory-backed block device, but not ATA registers or execution in CPL3.

## 0.8.0 / Partition-aware read-only mount
`StorageLab-GPT.img` is a disposable 10 MiB plaintext image with one 8 MiB Linux-filesystem GPT partition containing the existing demo EXT4 fixture. The block-slice layer validates inclusive partition bounds and all translated reads against the parent device. `mount-gpt 1` explicitly mounts the first enumerated validated Linux filesystem partition on the secondary IDE master, `umount` clears the single active mount, and existing `ls`, `cat` and `run*` use that mount. SwampVault partition types are rejected; no writes, multiple concurrent mounts, namespace isolation or transparent disk cryptography. `make run-storage-lab` attaches this fixture instead of the bare EXT4 volume. This is not a production filesystem or trusted-data mechanism.

## Overlord ELF profile extension
The previous single RX PT_LOAD remains valid. An optional second RW PT_LOAD at 0x40004000, max 4 KiB, allows data and BSS, with non-overlapping file intervals, fixed virtual addresses and NX enforced by the mapper. No dynamic loader. EXT4 remains read-only. See [OVERLORD](OVERLORD.md).
