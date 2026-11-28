<div align="center">

<img src="assets/banner.svg" alt="SwampOS Connectivity — connections have rules" width="100%" />

[![Release](https://img.shields.io/badge/release-v0.9.0--Connectivity-315c43?style=for-the-badge)](RELEASE.md)
[![Architecture](https://img.shields.io/badge/arch-x86__64-254c37?style=for-the-badge)](docs/ABI.md)
[![Kernel](https://img.shields.io/badge/kernel-C%20%2F%20ASM-315c43?style=for-the-badge)](docs/ARCHITECTURE.md)
[![Firmware](https://img.shields.io/badge/firmware-BIOS%20%2B%20UEFI_lab-315c43?style=for-the-badge)](docs/BOOT.md)
[![Stage](https://img.shields.io/badge/stage-EXPERIMENTAL-82734b?style=for-the-badge)](TEST-REPORT.md)
[![Network](https://img.shields.io/badge/network-VIRTIO_LAB-315c43?style=for-the-badge)](docs/NETWORK.md)

**A security-first operating system built from scratch.**  
No Linux kernel. No distribution remix. An independent bare-metal OS research project.

[Quick start](#quick-start) · [Hardware](#hardware-awakening) · [Boot](#three-independent-lab-paths) · [Storage](#swampfs) · [Networking](#network-foundation) · [Security](#onion-security-model) · [Roadmap](#roadmap) · [Release](RELEASE.md)

</div>

## `> whoami`

SwampOS is an independent security-first operating system project, built around a custom kernel, mandatory application isolation as a design goal, user-mediated hardware/network permissions, verified boot, encrypted storage, application compatibility and an open-source defensive software ecosystem. This release adds a bounded legacy VirtIO network laboratory, transport and configuration parsers, exact endpoint permission rules, and a fail-closed VPN route-decision model. The separate locked SwampLogin chain is unchanged.

| x86_64 | 60 KiB | 128 MiB | 5 |
| :---: | :---: | :---: | :---: |
| Architecture | BIOS payload limit | Managed RAM ceiling | Guest integration gates |

> [!IMPORTANT]
> **Experimental pre-release — not production security software.** QEMU/OVMF execution is not locally verified. The unsigned `KernelLab-GPT.img` and BIOS images are plaintext development paths. The original separate SwampLogin remains locked, never collects credentials, never releases keys and never boots the kernel. Do not enroll lab EFI executables as trusted, install these images on real disks, or store secrets in them. [Security boundaries](docs/SECURITY.md).

## What's new in 0.9.0

| Layer | Engineering delta | Evidence |
|---|---|---|
| UEFI | `KERNELLAB.EFI`: embeds raw kernel, fixed RAM reservation, memory map conversion, `ExitBootServices`, GDT/page-table handoff | PE build, host map tests, image extraction |
| BIOS | 120-sector load, 60 KiB payload, firmware-mode entry separation | Static image validation |
| CPU | CPUID features and processor metadata | Compiled kernel |
| ACPI | RSDP/RSDT/XSDT, MADT and MCFG checksum/length validation | Host fixtures + mutations |
| PCI | Controller classification, BAR decoding, PCIe/MSI/MSI-X/VirtIO capability inventory | Host config fixtures + mutations |
| Input | i8042 polling, US translated Set 1, modifier state | Host decoder tests |
| Display | GOP geometry/format metadata, UEFI serial-only console | Compiled firmware and kernel |
| Storage | Bounded partition block views; one read-only GPT/EXT4 mount; public demo image | Actual GPT/EXT4 host reads |
| Network | Opt-in transitional VirtIO PCI RX/TX, bounded Ethernet/ARP/IPv4/ICMP/UDP engine | Compiled driver, host queue tests; guest pending |
| Transport | TCP/IPv4, DNS, DHCP and basic IPv6 structural validators | 50,000 mutations |
| SwampWall | Exact per-app IPv4/protocol/port permission model, capacity and revocation | 50,000 identity checks |
| SwampVPN | Fail-closed route decision and offline URI metadata inspection | 32 route combinations, 10,000 URI mutations |
| Security | SwampLogin lock invariant retained, networking remains unavailable to applications | Previous host policy and login tests |

## Quick start

Linux / WSL2. Use GCC, GNU binutils, Clang, LLD, Python 3, libext2fs, cryptography, QEMU and OVMF.

```bash
(cd build && sha256sum -c SHA256SUMS)  # verify supplied artifacts before clean
./install.sh
make clean
make -j4 all
make test                 # host and static gates
make checksums            # local build manifest
```

`SHA256SUMS` belongs to the distributed build. `make clean` removes it; use `make checksums` after rebuilding to create a new local manifest (not an independent release authenticity proof). To verify the original download, run the checksum command before rebuilding.

Run the BIOS lab (plain EXT4 fixture, no network adapter):

```bash
make run
```

Run the new GPT-backed storage lab:

```bash
make run-storage-lab
# at the SwampShell prompt:
gpt
mount-gpt 1
ls /bin
cat /welcome.txt
run-console /bin/hello
umount
```

Guest validation is intentionally separate:

```bash
make runtime              # BIOS / bare EXT4
make uefi-runtime         # OVMF -> locked SwampLogin
make kernel-lab-runtime   # independent OVMF -> experimental kernel
make storage-lab-runtime  # BIOS -> GPT partition mount
make network-lab-runtime  # isolated QEMU virtio ARP / ICMP gate
```

The runtime commands require QEMU, and the UEFI commands require a matching non-Secure-Boot OVMF CODE/VARS template. They fail if prerequisites are missing; they never synthesize success. CI also runs these gates and retains any generated logs.

## Three independent lab paths

<img src="assets/hardware-map.svg" alt="Hardware Awakening, separate locked and plaintext laboratory boot paths" width="100%" />

| Image | Purpose | Important boundary |
|---|---|---|
| `SwampOS.img` | BIOS boot sector + raw x86_64 kernel | Plaintext lab only |
| `SwampBoot-GPT.img` | UEFI manager -> separate login partition | Stops locked; no handoff |
| `KernelLab-GPT.img` | Independent unsigned UEFI-to-kernel experiment | No credentials, empty ROOT/HOME |
| `StorageLab-GPT.img` | Public GPT-backed EXT4 demo | Attach as secondary IDE, no boot |
| `vault.ext4` | Existing native ELF / EXT4 fixture | Public, unencrypted, read-only |

```mermaid
flowchart TD
  B[BIOS lab] --> K[SwampKernel]
  U[Unsigned UEFI KernelLab] --> E[ExitBootServices / new GDT + map]
  E --> K
  S[SwampBoot manager] --> L[Separate SwampLogin]
  L --> D[LOCKED: no key backend]
  K --> H[Hardware inventory]
  K --> V[Read-only lab VFS]
  K --> N[Explicit VirtIO network lab]
```

The handoff is an experimental independent implementation, not a bypass of the locked manager: only the separate lab image contains it. The lab rejects Secure Boot enabled and LA57, reserves 1–4 MiB, copies the raw kernel to 1 MiB, builds a temporary 4 GiB identity map, converts only conventional RAM below 128 MiB, retries stale memory-map keys, and transfers without further firmware calls. Kernel memory management replaces the temporary map. Firmware execution remains unverified. [Boot specification](docs/BOOT.md).

## Hardware Awakening

| Command | Diagnostic or operation |
|---|---|
| `cpu` | Vendor, CPUID logical IDs and APIC/VMX/NX/SMEP/SMAP flags |
| `acpi` | Enabled processors, IOAPIC count, IRQ overrides, MCFG presence |
| `pci` | PCI bus/device/function and IDs |
| `devices` | AHCI/NVMe/xHCI/Ethernet/display/VirtIO classes and capabilities |
| `firmware` | BIOS/UEFI mode; GOP width, height, stride and format in UEFI |
| `pmm` / `vmtest` | Frame statistics and address-space self-test |

The PCI inventory layer reads configuration space without enabling bus mastering or programming BARs; the separate opt-in network lab configures one transitional VirtIO NIC and enables DMA only during its bounded session. ACPI tables are bounded and checked before traversal. Inventory is **not** driver support; no APIC routing, SMP, AHCI, NVMe, xHCI, modern VirtIO transport, USB, mouse, graphics output, IOMMU or CUDA runtime is implemented. PS/2 input is a polling path for firmware-translated Set 1 only. [Hardware details](docs/HARDWARE.md).

## SwampFS

```text
ATA PIO -> bounded block device -> validated GPT -> partition view
         -> constrained EXT4 reader -> single VFS mount -> native ELF64
```

- 512-byte sectors; 4 KiB EXT4 blocks; up to 64 KiB file size; one active read-only mount.
- Validated primary and backup GPT with CRC32 and nonoverlapping partition checks.
- Explicit `mount-gpt N` for Linux-filesystem GUID only; SwampVault GPT types are rejected.
- No filesystem writes, journal replay, multi-mount namespaces or transparent block encryption.
- External ELF is restricted to the existing native test ABI; explicit console grants remain required.

[Storage specification](docs/STORAGE.md) · [ABI](docs/ABI.md)

## Network foundation

<img src="assets/connectivity.svg" alt="Explicit VirtIO lab and separate security policy models" width="100%" />

| Protocol / layer | Current implementation | Not implemented |
|---|---|---|
| VirtIO PCI | Transitional legacy device `1af4:1000`, MAC feature, split rings, polling, explicit stop | Modern transport, IOMMU, physical NICs, guest validation |
| Ethernet / ARP | Local decode and reply, strict lengths | Neighbor cache |
| IPv4 / ICMP | Local checksum and echo reply | Routing, reassembly |
| UDP | Checksummed port-7 echo | Socket API |
| TCP | IPv4 header, checksum and option validation | Connection state, retransmissions, stream API |
| IPv6 | Fixed-header structural checks | Extension headers, transport checksums, NDP, traffic |
| DNS / DHCP | Bounded message parsing | Resolver, leases, client state |
| SwampWall | Exact app/IPv4/protocol/port grant table | Enforcement on application sockets |
| SwampVPN | Route-decision model and offline URI inspection | Tunnels, Xray/sing-box integration, auto-discovery |

### Run the isolated network gate

```bash
make connectivity-host-test
make network-lab-runtime
```

The latter starts a QEMU guest with a loopback-only socket peer and an explicitly selected transitional VirtIO adapter; it sends ARP and ICMP fixtures and verifies replies. No host bridge, NAT, Internet or automatic network activation. `netlab` is an explicit kernel-console operation, only for a trusted TCG/KVM laboratory, with a bounded polling interval and reset/disable of bus mastering on exit. This is not a hardware safety boundary; DMA is not IOMMU-isolated. Do not run this on physical hardware.

### VPN profile inspection (host only)

```bash
python3 tools/vpn_inspect.py local-profiles.txt
```

Recognizes a narrow URI metadata subset of VLESS, Hysteria2, Trojan and TUIC. No remote fetching, DNS lookup, connection attempts, protocol-specific credential validation or usable configuration generation. Userinfo, query values and labels are omitted from the report; endpoint metadata is still potentially sensitive. The route model denies ungranted endpoints and tunnel-required traffic when no tunnel is ready; it is not wired to real packet routing. [Network design](docs/NETWORK.md).

## Onion security model

<img src="assets/memory-map.svg" alt="Experimental separate process memory layout" width="100%" />

```text
HARDWARE -> SWAMPKERNEL -> ONION POLICY -> SWAMPBOX -> APPLICATION
                |               |              |
             PMM/VMM       capabilities    separate roots
```

Existing experimental Ring 3, TSS, interrupt-gated syscalls, NX, guard page, default-deny console grants, watchdog and scrubbed user frames are retained. The kernel uses a shared supervisor physical mapping; this is not physical-frame-wide W^X. Application isolation is not yet a production sandbox. No DMA isolation, preemptive user process scheduler, SMP, or general device permission broker exists.

### SwampVault and verified login — target, not yet integrated

<img src="assets/vault-flow.svg" alt="Offline RSA and AES encryption prototype" width="100%" />

The protected target remains **encrypted EXT4 ROOT/HOME unlocked only after user authentication in an independent signed SwampLogin partition**. AES-XTS block encryption with designed integrity protection, RSA-OAEP key wrapping, Argon2id, TPM sealing and signed recovery are architecture targets. Existing host `tools/vault.py` performs RSA-4096/AES-256-GCM whole-image envelope authentication; `tools/boot_registry.py` signs/encrypts offline registry records. These are not kernel disk encryption, not a native login backend, and not hardware anti-rollback. The default locked UEFI image has empty ROOT/HOME and accepts no passwords.

[Vault](docs/VAULT.md) · [Boot chain](docs/BOOT.md) · [Security](docs/SECURITY.md)

## SwampShell

Illustrative target transcript — **not a captured boot log**:

```text
SwampOS 0.9.0 Connectivity
Security has layers.
BOOT_OK
swamp> firmware
UEFI plaintext kernel lab
swamp> net
Ethernet / ARP / IPv4 / ICMP / UDP. NIC only in explicit netlab; no sockets or Internet.
swamp> nettest
NET_TEST_OK
swamp> transport
TRANSPORT_TEST_OK
swamp> wall
WALL_TEST_OK
swamp> vpn-policy
VPN_POLICY_TEST_OK
swamp> shrek
WHAT ARE YOU DOING IN MY SWAMP?!
```

New commands: `netlab` (trusted-emulator only), `transport`, `wall`, `vpn-policy`. Other commands: `help`, `clear`, `version`, `status`, `onion`, `mem`, `ticks`, `tasks`, `selftest`, `sandbox`, `fscheck`, `storage`, `ls`, `cat`, `run`, `run-console`, `halt`. `BOOT_OK` is gated by kernel VM/user/core/network/connectivity tests and the storage test when a demo volume is mounted.

## Five editions — one shared kernel

| Edition | Intended role | Current state |
|---|---|---|
| Desktop | Daily use and graphical applications | Profile manifest only |
| Defence | Authorized defensive security, network/forensics tools | Profile manifest only |
| Developer | SDK, compilers, language runtimes | Profile manifest only |
| Privacy | VPN, per-app routing, encrypted storage | Profile manifest only |
| **Full** | Union of the four profiles | Profile manifest only |

## Roadmap

| Subsystem | Evidence / next gate |
|---|---|
| SwampKernel / Donkey | Freestanding C/ASM, memory/scheduler host tests; guest runtime pending |
| SwampBox / Onion | Experimental Ring 3 and policy; guest and independent audit pending |
| SwampFS | Read-only EXT4 and single partition mount; writes and full VFS pending |
| SwampBoot / SwampLogin | Separate locked chain, unsigned independent UEFI lab; native authentication pending |
| SwampVault | Offline encryption, native encrypted block I/O pending |
| Hardware | Discovery and limited PS/2; experimental legacy VirtIO NIC; APIC/SMP/USB/storage/GPU drivers pending |
| SwampNet / SwampWall | Legacy VirtIO lab, structural transport parsers, grant model; actual guest networking and socket enforcement pending |
| SwampVPN | Offline URI metadata and route model; Xray/sing-box, VLESS/REALITY, Hysteria2, WireGuard, OpenVPN and actual per-app routing planned |
| SwampPKG | Signed `.swp`, DEB/RPM import, immutable/atomic updates planned |
| SwampUnix / SwampWin | Linux ABI, Win32/EXE compatibility planned |
| SwampCompute | CUDA/ROCm/oneAPI, mediated GPU and isolated guest compute planned |
| SwampGUI / SDK | Desktop, native apps, Rust/toolchains and developer ecosystem planned |
| Full Edition | Integration, installation, recovery and independent audit planned |

The detailed target remains in [ARCHITECTURE.md](docs/ARCHITECTURE.md). Third-party driver and application ports must preserve license/attribution. Project license is not yet finalized; do not assume the release is already under GPL or that proprietary CUDA components are redistributable.

## Test evidence and repository

See [TEST-REPORT.md](TEST-REPORT.md) for actual local test runs and [CHANGELOG.md](CHANGELOG.md) for history. Build scripts are in `Makefile`, CI in `.github/workflows/`, tests in `tests/`, boot code in `boot/`, kernel code in `kernel/`, independent applications in `userspace/`, provisioning/crypto in `tools/`, assets in `assets/` and earlier documents in `legacy/`. The release ships full source, compiled images, manifests, notes, diagrams and test logs. [Tree](TREE.txt).

<div align="center">

**SWAMP MATRIX // SWAMP OS**  
`YOUR SYSTEM. YOUR RULES. YOUR SWAMP.`

</div>
