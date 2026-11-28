# Build 38 revision 1 verification

The x64 hybrid UEFI Live ISO contains 44,980,224 bytes. SHA-256 `2558f4083259eada94a59bf9a7243f62888f2b2cc021987a193854ad53dc641d`. Component sizes are in `build/Farkuad-Live.iso.report`.

| Gate | Evidence |
|---|---|
| AHCI and NVMe | Seven Q35/OVMF boots per transport on 4 MiB images. Two successive writes, abrupt VM termination, cold boot, unmount/remount and exact payload checks passed. |
| Torn latest record | Host damaged 80 bytes after shutdown. Both guests selected the preceding valid generation and committed a replacement. The host checked all 4 MiB. |
| Refusal and isolation | Missing opt-in, absent VT-d, wrong serial and wrong label each prevented writes. All eight images matched the originals byte for byte. |
| Host failures | Rejected equal generations, wrap, invalid length and characters. Injected write, flush and readback errors each required a remount. |
| Regression | Aggregate host tests, optical and USB-media Live boots, locked UEFI manager and paired xHCI input passed. |
| Packaging | Retained historical archive paths and logs. Protected EFI and GPT hashes matched Build 37. The extracted component and deliverable SHA-256 manifests matched. |

Revision 1 adds host coverage for a 1,024-entry VirtIO split ring. The guest exchanged ARP and ICMP packets over an isolated QEMU link, then shut the device down. The BIOS shell, UEFI kernel lab, protected manager, storage lab, GUI framebuffer, Live desktop and USB pair also passed locally. The independent KernelLab GPT guest booted with 256 MiB on Ubuntu QEMU 10.2.1 and OVMF 2025.11. All fourteen storage boots, the USB input pair and the IOMMU probe passed on the Ubuntu packages with the versioned pc-q35-8.2 machine and 128 MiB. Revision 1 logs use the prefix `build/logs/build38-r1-`. The original Build 38 logs remain in the archive. The automated tests used headless QEMU. Physical hardware, VirtualBox storage and the GTK storage launcher window were not tested.

## Commit from the integrated checkout

```sh
git switch -c build/38-persistent-storage
git add -u
git add -- kernel tests docs/PERSISTENCE.md
python3 scripts/check-git-index.py && git commit -m "storage: persist a guest file through AHCI and NVMe"
```

---

## Build 37 verification (historical)

Clean source build on Debian with GCC 14.2, Clang/LLD 19.1.7 and QEMU 10.0.13 / OVMF. The Live ISO SHA-256 is `d480878ebf9de1b3400407a4bb15d7a3142412d984fd0bfffebeee6e1942594a`. The size is 44,972,032 bytes. The component report is in `build/Farkuad-Live.iso.report`.

| Command / profile | Result | Observed behaviour |
|---|---|---|
| `make -j4 all` in an empty build directory | 0 | Kernel, EFI applications, lab images and the hybrid ISO were generated |
| `make -j4 test` | 0 | Full host regression, including USB rings, MMIO isolation, renderer, parser and image checks |
| `make live-usb-input-runtime` — pair | 0 | Keyboard typed version, USB Tab opened Notes, USB mouse moved the cursor and dragged/resized the window, USB Esc restored Terminal |
| Keyboard-only and mouse-only profiles | 0 | Each device worked independently on the same Live ISO |
| No VT-d, missing opt-in, three connected devices | 0 | Three explicit activation refusals, shell remained available |
| `make live-runtime live-optical-runtime` | 0 | USB-media and optical boot with the existing PS/2 desktop, native ELF, read-only storage checks |
| `make live-classic-runtime live-recovery-runtime` | 0 | Classic desktop and serial recovery |
| `make live-iommu-runtime uefi-runtime` | 0 | Read-only firmware inventory and the separate locked SwampLogin boot chain |
| `make checksums` / `sha256sum -c` | 0 | Bundle and image manifests matched |

## Guest evidence

Q35 used 128 MiB, optical boot, i8042 disabled, and one xHCI controller. The test injected events through QEMU, observed the shell response and compared guest pixels at 1280 × 800. Notes moved by 24 × 16 logical pixels and its resize grip shifted by 36 × 20. Each motion frame matched a pointer-free background composited with the expected cursor.

The monitor read the VT-d translation status and guest-physical table entries. The xHCI BDF was the only present requester. With both devices, seven sparse windows contained twelve distinct pinned pages. Every other leaf and the table pages were outside the DMA domain. Each positive profile halted and restarted the driver, verified an empty root after revocation, then unplugged a device and observed session teardown.

The physical input path and VirtualBox graphics still require a separate run. The local GTK frontend requires a graphical QEMU installation. Automated guest framebuffer tests used QEMU's headless display. A separate desktop-window smoke test could not start an X11 display in this environment because the X server could not create its local listening sockets.

## Commit from the integrated checkout

```sh
git switch -c dev/build-37-usb-input
git lfs install --local
git add -A -- .
git add -f -- build/logs
python3 scripts/check-git-index.py && git commit -m "usb: drive keyboard and mouse through a VT-d isolated xHCI controller"
git lfs fsck
```

## Commit from the integrated checkout

```sh
git switch -c build/38-persistent-storage
git add -u
git add -- kernel tests docs/PERSISTENCE.md
python3 scripts/check-git-index.py && git commit -m "storage: persist a guest file through AHCI and NVMe"
```
