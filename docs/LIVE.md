# Awakens / Live boot guide (Farkuad-derived)

**Status: experimental removable-media build.** Farkuad is an unsigned UEFI x86-64 kernel lab with a desktop with experimental PS/2 keyboard and mouse support. It is not a secure installed OS. The independent locked SwampLogin chain is unchanged; Farkuad never unlocks a vault. No installation, disk writes, persistent notes across reboot, or automatic networking are implemented in the live route. Prefer an isolated VM first; physical tests are for a spare, non-production computer with a PS/2-compatible keyboard or serial console.

## Build the image (Linux or WSL2)

```sh
./install.sh                  # Debian/Ubuntu dependencies
bash scripts/build-live.sh    # build and host-test; no device access
```

The result is `build/Farkuad-Live.iso`, `build/Farkuad-Live.iso.sha256` and `build/Farkuad-Live.iso.report`. ISO size is payload-dependent; Build 35 retains payload-sized FAT16/FAT32 volumes; see `build/Farkuad-Live.iso.report` for the measured image size. The full release archive contains all three. `make live` builds the hybrid ISO and report. `make live-host-test` verifies the host components, ISO9660, El Torito and both FAT boot volumes. `make live-runtime` attempts the real guest USB boot in QEMU/OVMF; it deliberately fails if a dependency or expected marker is missing. Do not equate host tests with firmware validation.

```sh
(cd build && sha256sum -c Farkuad-Live.iso.sha256)
make live-runtime
make live-optical-runtime
```

## Hybrid ISO layout

The ISO9660 root contains an El Torito EFI boot catalog, a payload-sized FAT16 optical boot image and a payload-sized FAT32 USB ESP. The MBR partition entry points directly to the FAT32 extent inside the ISO. Both volumes contain the exact same `EFI/BOOT/BOOTX64.EFI` payload. FAT16/El Torito imposes a 30 MiB EFI payload ceiling in this builder; FAT32 also enforces its 65,525-cluster minimum. The build prints exact component sizes and writes `build/Farkuad-Live.iso.report`. Flash in raw/DD mode; do not extract ISO files manually. UEFI x64 only, no BIOS El Torito path.

## Write to USB on Windows

1. Extract the complete release. The file to flash is `build/Farkuad-Live.iso`, **not** the ZIP, `SwampOS.img`, `SwampBoot-GPT.img` or any other lab image.
2. Use a disposable USB stick. Back up its contents first. Verify the SHA-256 with `Get-FileHash .\build\Farkuad-Live.iso -Algorithm SHA256` and compare against the supplied checksum.
3. In a raw-image writer such as balenaEtcher, select the ISO, select the USB stick by capacity/model, flash and verify. If using a tool that offers DD/raw mode, choose that mode. Flashing erases the USB stick; do not choose a system disk. No copying the ISO as an ordinary file and no ISO conversion are necessary.
4. On a spare x64 PC, use the one-time UEFI boot menu, select the USB device in UEFI mode. The Farkuad EFI binary is unsigned and intentionally refuses Secure Boot enabled. Use a test machine already configured for unsigned UEFI boot; do not change security settings on your main or managed PC. Changes to firmware security settings may trigger BitLocker recovery. Never enroll the lab image as a trusted production loader.
5. If the desktop does not appear, note the firmware message or serial output and stop. No boot on a real PC is guaranteed. The xHCI work remains host-modelled and is not connected to a physical controller; native USB HID and GPU drivers are not present. Do not install to an internal disk.

## Linux raw flashing (optional)

```sh
lsblk -o NAME,SIZE,MODEL,TRAN,RM,MOUNTPOINTS
# Unmount the selected USB's partitions manually first.
sudo bash scripts/flash-live.sh /dev/sdX
```

Replace `/dev/sdX` with the whole disposable USB disk, **not** a partition. The script refuses non-removable/non-USB disks and mounted descendants, asks for the exact device path as a destructive confirmation, writes the image, then compares it to the device. It is not intended for WSL or for USB disks reporting RM=0. It has not been exercised on physical media in this release.

## Firmware and hardware requirements

| Requirement | Current design |
|---|---|
| CPU | x86-64 with NX; no ARM or 32-bit EFI |
| Firmware | x64 UEFI, unsigned boot allowed; MBR ESP type EF/FAT32 for USB; El Torito EFI/FAT16 for CD/DVD |
| Memory | Aim for >= 256 MiB installed; kernel manages only first 128 MiB of eligible physical memory |
| Low memory | Firmware must permit fixed 1–4 MiB kernel reservation and 8 MiB RAM-disk allocation below 128 MiB |
| Display | GOP linear framebuffer, RGB/BGR 32-bit, 640x360 through 4096x4096 within 64 MiB mapping cap; unsupported formats fall back to serial |
| Input | Explicit i8042 initialization, PS/2 set-1 keyboard (US layout), relative PS/2 mouse, optional COM1; no native USB HID |
| Timer | Legacy PIC/PIT required for existing self-tests and scheduling; not guaranteed on modern platforms |
| Storage | Boot firmware loads EFI; embedded 8 MiB read-only EXT4; no native USB storage/NVMe access in live mode |

## Desktop tools

Boot option 1 opens the window manager with a launcher, taskbar, mouse drag/resize and window controls. Option 2 keeps the classic keyboard-only interface. Tab cycles Terminal → Notes → Calculator → Monitor → Files. Escape returns to Terminal. The terminal runs the existing real kernel shell. Notes are volatile, up to 512 bytes, editable with Backspace. In windowed mode Save/Load uses the RAM file notes.txt; terminal ram* commands access the same namespace. Calculator accepts signed integer operands up to nine digits, one operator (+ - * /), Enter evaluates, integer division truncates toward zero. Monitor updates timer ticks and physical frame counts. Files uses the mounted RAM EXT4: press 1 for welcome text, 2 for binary hex preview, 3 for `/bin` directory. Terminal supports arbitrary supported read-only paths through `ls` and `cat`.

```text
help
version
cpu
devices
pmm
process-test
audit
storage
ls /bin
cat /welcome.txt
run-console /bin/sdk-demo
run-pair /bin/overlord-demo
```

`netlab` is not dispatched in Live mode, and `gpt`/`mount-gpt` are rejected before ATA access. `umount` and `mount-live` manage the RAM mount. `halt` stops CPU execution; turn off the test machine manually. Desktop tools are trusted kernel components, not isolated third-party programs. Existing native ELF applications retain experimental capability enforcement and are not given permissions automatically.

## Architecture

The standalone `FARKUAD.EFI` embeds the kernel and the exact `vault.ext4` artifact. The UEFI loader copies the kernel into its reserved region, copies the filesystem into reserved loader pages below 128 MiB, records framebuffer metadata, obtains a firmware memory map, exits Boot Services, and transfers control to SwampKernel. The kernel creates 4 KiB page tables and maps the framebuffer at a supervisor-only, NX high virtual address (PML4 index 1). The branch is shared by user roots without U/S permission. The RAM-disk adapter rejects invalid ranges and overlapping firmware usable ranges; VFS mounts EXT4 read-only. The display is redrawn by the same C renderer tested on the host. No UEFI graphics services are called by the kernel after exit.

This path intentionally does **not** claim encryption, Secure Boot, a persistent package installation system, a full compositor or mandatory isolation of desktop tools. Those remain release gates for the secure installed edition.

## VirtualBox

Use EFI x64, PS/2 Mouse, 1 CPU, 512–1024 MiB RAM, an optical ISO attachment, no 3D acceleration and no network. Do not select USB Tablet. See [INPUT.md](INPUT.md) for the menu and diagnostics.
