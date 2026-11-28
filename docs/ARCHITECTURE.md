# Target architecture (not implemented)
- x86_64 hybrid kernel, Rust no_std, C/C++ and assembly; ring 3, paging, scheduler, IPC, capability enforcement, IOMMU, audited user-space drivers. SDK for C/C++, Rust, Python, Go, Java, JS/TS, Lua, Ruby, PHP, Perl, Bash, Zig, .NET and WASM.
- SwampBox: mandatory default-deny per-app sandbox including installers; mediated USB, network, LAN, files, camera, microphone, clipboard, screen, GPU, child processes; resource quotas, revocation, ephemeral profiles and microVMs.
- SwampBoot: UEFI Secure Boot, authenticated encrypted multiboot registry, TPM-sealed keys, anti-rollback, signed recovery, chainload verified OS loaders. Legacy BIOS bootstrap is not secure boot.
- SwampLogin: separate signed preboot partition, offline authentication before unlocking root/home. EXT4 inside encrypted block volumes; AES-XTS with separately designed integrity protection, RSA-OAEP key wrapping, Argon2id, TPM plus user authentication. Encrypted swap and secure logout. No custom cryptographic primitives. LUKS2 interoperability requires verification.
- SwampPKG: signed .swp packages, manifests, SBOM, dependencies, atomic updates, rollback; DEB/RPM import and Linux ABI, PE/EXE/MSI via Win32 compatibility or isolated guests. Package parsing does not imply execution compatibility.
- SwampVPN: port Xray/sing-box, VLESS/REALITY, VMess, Hysteria2, TUIC, Trojan, Shadowsocks, WireGuard, OpenVPN, SOCKS, HTTP proxies, JSON/YAML/subscriptions/QR; discover publicly offered free configs, validate in isolated environment; explicit opt-in, per-app routing, kill switch, DNS/IPv6 leak protection.
- Drivers: evaluate Linux/BSD/Haiku/ReactOS, Mesa, ACPICA, EDK II, VirtIO; preserve licenses and attribution; port PCI, USB, NVMe, SATA, Ethernet, Wi-Fi, Bluetooth, audio, input, GPU. No upstream code imported yet.
- SwampCompute: CUDA, cuDNN, TensorRT, ROCm/HIP, oneAPI, OpenCL, Vulkan, NVENC/NVDEC, OptiX; mediated GPU or IOMMU passthrough to Linux guest where supported. Proprietary components require licensing review.
- Desktop: SwampGUI, Files, Terminal, Browser, Store, Guard, Monitor, Code, Notes, Office, PDF, Paint, Media, Image, Mail, Calc, Calendar, Archive, Capture, Backup, Disk, Remote, Settings, Update, Recovery, Search, Torrent, Download.
- Defence: Net, Packets, DNS, Ports, Proxy, Firewall, Forensics, Hex, Disasm, Debug, CVE, Audit, Logs, Hash, Cert, Malware analysis, Lab, Vault, Crypto, OSINT; evaluate existing open-source tools for authorized defensive use.
- Profiles: Desktop, Defence, Developer, Privacy, Full (union). Shared kernel and mandatory security policy. English UI, muted dark green, monospace, meme commands.

## Next gates
Long mode and UEFI; physical/virtual memory; IDT and timer; ring 3 and syscall isolation; storage and verified login; network and drivers; compatibility; desktop and application ports. Each gate requires runtime tests. No production security claims before audit.

## 0.5.0 implementation delta
E820 collection, capped physical allocator, 4 KiB page tables, kernel segment permissions, separate user roots, TSS, Ring 3 test payloads, syscall gate, limited capability checks and watchdog are now in source. Host validation passed; guest execution is pending. See ABI.md and SECURITY.md for exact boundaries. The target architecture above is retained, not a claim that the remaining systems work.

## 0.6.0 implementation delta / Vault
Read-only block bounds, secondary IDE ATA PIO driver, narrow EXT4 parser and path lookup, independently built native ELF64 application on a libext2fs-generated disk, operator-controlled launch through the existing sandbox, and offline host RSA/AES image envelope are added. See STORAGE.md and VAULT.md. No actual VFS mount layer, encrypted block device, verified login partition or UEFI boot is present. Existing capabilities, five editions and full architecture targets remain in scope.

## 0.7.0 implementation delta / SwampBoot
Separate x64 UEFI PE applications, strict/lab firmware policy, same-disk login chainload, locked login UI, GPT/FAT32 provisioning, strict read-only GPT parser and legacy PCI inventory. Offline signed/encrypted multiboot registry and encrypted-envelope provisioning are added. No UEFI-to-kernel handoff, native preboot authentication, Secure Boot signatures/enrollment, TPM anti-rollback or transparent block encryption. BIOS/UEFI runtime validation remains pending. See BOOT.md.

## 0.8.0 implementation delta / Hardware Awakening
The independent plaintext KernelLab EFI image embeds the same freestanding kernel and attempts a bounded `ExitBootServices` handoff with conservative conventional-memory conversion and a new GDT/identity map. The original locked UEFI manager/login branch does not invoke it. CPU feature discovery, ACPI RSDP/RSDT/XSDT and MADT/MCFG inventory, GOP metadata, read-only PCI BAR/capability classification, and a translated PS/2 keyboard polling path are added. A single-mount read-only VFS supports explicit Linux-filesystem GPT partition views; offline Ethernet/ARP/IPv4/ICMP/UDP processing has no NIC transport. See HARDWARE.md, NETWORK.md and BOOT.md. APIC programming, SMP, AHCI/NVMe/xHCI/VirtIO drivers, TCP, sockets, real application networking, verified authentication and native disk encryption remain pending. Hardware runtime is not established by host testing.

## 0.9.0 implementation delta / Connectivity
An opt-in transitional VirtIO network lab and split-ring handling, bounded TCP/IPv4, DNS, DHCP and IPv6 parsers, exact endpoint permission model, VPN route-decision model and offline URI metadata inspector are added. The NIC driver is compiled but not executed; no IOMMU, real application sockets, TCP state machine, DNS resolver, DHCP client, VPN tunnel or live routing. See NETWORK.md. Earlier boot/storage and separate login invariants are preserved; hardware drivers, authentication and encrypted block I/O remain targets.

## 0.10.0 implementation delta / Ecosystem
Host-staged signed `.swp` format with exact dependencies, atomic activation and one snapshot rollback; structural ELF/PE/DEB/RPM inspection; minimal freestanding C SDK and separately compiled ELF on the demo EXT4; kernel bounded audit ring; independent static GOP graphics lab using a host-tested software renderer. See PACKAGES.md, SDK.md, AUDIT.md and GUI.md. Not native package installation, foreign ABI support, full GUI, or complete SwampGuard. The existing locked SwampLogin and offline/default-deny boundaries remain unchanged. The five editions are still profile manifests, not installable editions.

## 0.11.0 implementation delta / Integration
Per-run identifiers and session-scoped data snapshots extend the native syscall ABI. A signed-store export tool connects host verification with the public EXT4 fixture; guest does not verify package signatures or install packages natively. Git LFS image tracking and CI rebuild rules are included. No persistent PID, concurrent user tasks, IPC, full desktop, encrypted block I/O or native authentication implemented.

## 0.12.0 Overlord
Bounded user process groups, preemptive round-robin, per-slot capabilities, IPC, private scratch, optional RW/NX ELF data page and expanded native ABI. See [OVERLORD](OVERLORD.md). Existing encryption, boot, hardware and compatibility targets remain.

## Farkuad Live route (0.13.0)

Separate unsigned removable-media EFI entry: `FARKUAD.EFI` embeds `kernel.bin` and `vault.ext4`, allocates firmware-reserved low memory, and hands off to the existing custom kernel. The desktop uses a supervisor-only NX framebuffer mapping at `0x8000000000` and a bounded read-only RAM-disk adapter. The live route does not dispatch the NIC lab or physical ATA/GPT mounts. No authentication/unlock or production security claim. See `LIVE.md` for detailed limits and flash procedure.

## Awakens internal builds 01–02

`desktop/window.c` provides five overlapping kernel tool windows and input/focus state. `filesystem/ramfs` provides a separate writable, volatile, flat namespace used by Notes and the shell. These additions do not change the read-only EXT4 or locked boot chain. See [AWAKENS.md](AWAKENS.md).


## Awakens Build 15 — volume-scoped guard profiles

The narrow EXT4 mount extracts its 128-bit superblock UUID. SwampGuard keys volatile launch ceilings by `(UUID, inode)`, so equal inode numbers on different UUID volumes do not collide, while hard links and remount aliases on the same filesystem retain a shared ceiling. Zero-UUID filesystems are rejected. Duplicate UUID clones share policy conservatively. This is not persistent or authenticated identity; see [GUARD-IDENTITY.md](GUARD-IDENTITY.md).
