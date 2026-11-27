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
