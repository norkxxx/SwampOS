# Hardware Awakening | Device inventory and input

| Interface | Code in 0.8.0 | Guest status |
|---|---|---|
| CPUID | Vendor, logical-ID count, APIC/VMX/NX/SMEP/SMAP flags | Not run |
| ACPI | RSDP v1/v2, RSDT/XSDT checksum, MADT local APIC/x2APIC/IOAPIC/IRQ override inventory, MCFG presence | Host fixtures only |
| PCI | Legacy configuration access, header-aware BAR decode, 64-bit BAR pairs, bounded capabilities with cycle detection, PCIe/MSI/MSI-X/VirtIO vendor capability indicators | Host fixtures only |
| Controllers | Classify AHCI, NVMe, xHCI, Ethernet, display, VirtIO; no binding | Host fixtures only |
| Input | Read-only polling i8042 data/status, translated US Set 1, shifts, caps, ignore auxiliary/extended sequences | Decoder host-tested, hardware not run |
| GOP | Firmware resolution, stride, pixel format metadata in lab bootinfo | Not run |

`cpu`, `acpi`, `devices`, `pci`, `firmware` are shell diagnostics; normal keyboard typing is accepted along with serial input when firmware has left a translated PS/2 keyboard available. No keyboard controller initialization or USB HID implementation; no mouse. The ACPI parser uses a bounded physical-reader interface, validates RSDP/root/table checksums, caps table lengths at 64 KiB, validates MADT entry sizes and bounds and reports enabled processors only. BIOS searches recorded EBDA/ROM; UEFI uses the firmware configuration table pointer. Tables outside the low identity-mapped 128 MiB window are not available. No AML interpreter, interrupt routing changes, AP startup or ECAM access. PCI BAR addresses are decoded without size probing, writes or MMIO mapping; classification is not a driver. No upstream driver code imported yet. Native driver ports need licensing review and an IOMMU design before DMA.

## 0.9.0 addition
The explicit `netlab` trusted-emulator path binds a narrow transitional VirtIO PCI network device with legacy I/O registers, split queues and static DMA buffers. It is not part of read-only inventory. No modern VirtIO, IOMMU, hardware runtime verification, AHCI/NVMe/xHCI driver, APIC/SMP or GPU support. See NETWORK.md for the precise scope and lab-only constraints.
