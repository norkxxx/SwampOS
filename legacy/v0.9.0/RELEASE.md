<div align="center">
<img src="assets/banner.svg" alt="SwampOS Connectivity" width="100%" />

# SwampOS v0.9.0 — Connectivity

**Security has layers. Connections have rules.**

Experimental pre-release | x86_64 | C / ASM
</div>

Connectivity extends Hardware Awakening with an opt-in legacy VirtIO network lab, bounded protocol parsers, and the first network permission and VPN decision models. All earlier source, profile manifests, boot paths, disk images and architecture goals are retained. The protected SwampLogin chain remains locked.

## Engineering changes

- Transitional VirtIO PCI Net: MAC feature only, polling, split RX/TX queues, static DMA buffers, bounded session and reset/disable on exit. Only an explicit console command in a recognized trusted emulator can start it. The isolated QEMU gate uses a loopback socket peer, not a bridge/NAT.
- TCP/IPv4 checksum/header validation, DNS message parsing with bounded compression, DHCP response TLVs and basic IPv6 fixed-header parsing. These are validators, not complete transport or client implementations.
- SwampWall exact app/IP/protocol/port permission table with revocation; SwampVPN fail-closed route decision model and offline metadata inspection for VLESS, Hysteria2, Trojan and TUIC URIs. No actual application networking or VPN tunnel.
- Preserve the original separate locked UEFI login, experimental UEFI-to-kernel laboratory, BIOS bootstrap, read-only GPT/EXT4, native ELF, Ring 3, allocator and cooperative scheduler.

## Evidence

Two clean builds produced matching SHA-256 for all 13 distributed artifacts. One full regression run and two runs of the new connectivity suite completed. New tests per run: 50,000 protocol mutations, 50,000 identity checks, 150,000 queue cycles, 32 VPN decisions and 10,000 URI mutations. See [TEST-REPORT.md](TEST-REPORT.md) and `build/` logs.

## Run

```sh
(cd build && sha256sum -c SHA256SUMS)
./install.sh
make clean
make -j4 all
make test
make network-lab-runtime
```

The five guest gates need QEMU; UEFI additionally needs OVMF. They were attempted locally but cannot execute because QEMU is absent. No physical hardware test or independent security audit. No IOMMU, TCP connections, actual sockets, VPN tunnel, native full-disk encryption or pre-boot authentication. Default lab images are plaintext; do not install them on real disks or store secrets in them.

**Tag:** `v0.9.0` · **Type:** Pre-release

`YOUR SYSTEM. YOUR RULES. YOUR SWAMP.`
