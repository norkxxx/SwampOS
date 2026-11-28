<div align="center"><img src="assets/banner.svg" alt="Hardware Awakening" width="100%" /></div>

# SwampOS v0.8.0 — Hardware Awakening | The Swamp Wakes Up

> Experimental pre-release. Host-tested; guest CPU and firmware execution are not verified locally.

The eighth SwampOS release expands the custom x86_64 kernel beyond the previous firmware front end. An **independent unsigned UEFI kernel laboratory** implements a tentative `ExitBootServices` handoff, while the separate locked SwampLogin path still releases no keys. The release also adds ACPI/PCI hardware discovery, CPUID and PS/2 input, read-only GPT partition mounting, and an offline Ethernet/ARP/IPv4/ICMP/UDP engine.

### Highlights
- Independent `KERNELLAB.EFI` and `KernelLab-GPT.img` with bounded RAM conversion and explicit firmware-mode entry; BIOS load capacity grows from 32 to 60 KiB.
- CPU features, ACPI MADT/MCFG, read-only PCI BAR/capability discovery, translated PS/2 input and GOP metadata.
- New `StorageLab-GPT.img`, partition slice checks, explicit read-only mount/unmount and existing external ELF pipeline.
- Offline network protocol fixtures, default-deny network state and extended test/CI gates.
- All previous subsystems, five edition manifests, offline encryption tooling and locked preboot architecture retained.

### Validation
See [TEST-REPORT.md](TEST-REPORT.md) for actual host results and reproduction commands. Guest testing requires QEMU/OVMF. No production boot, encrypted login, hardware driver support or network connectivity claim is made. Do not install on real disks or store sensitive data.

### Distribution
Attach `SwampOS-v0.8.0-Hardware-Awakening.zip` to GitHub Releases. Mark **pre-release**, tag `v0.8.0`. Source, compiled artifacts, documentation and branded SVG assets are all included.

**Security has layers. The swamp is waking up.**
