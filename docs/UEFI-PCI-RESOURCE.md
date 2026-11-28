# UEFI PCI resource handoff — Build 33

## Protocol path

The Farkuad UEFI loader locates handles supporting `EFI_PCI_IO_PROTOCOL`, obtains the interface with `HandleProtocol`, and reads the PCI location/configuration. It selects only class `0C:03:30` (xHCI), endpoint header type 0, and a memory BAR0. It requests current BAR resources with `GetBarAttributes(This, 0, &Supports, &Resources)`. The returned `Resources` buffer is released with Boot Services `FreePool` on success and error paths.

## Descriptor validation

The resource parser accepts ACPI QWord (`0x8A`) and DWord (`0x87`) Address Space descriptors and the End Tag (`0x79`). It checks descriptor lengths, optional trailing ResourceSource bytes, EndTag checksum, memory resource type, zero translation offset, BAR base range, overflow, 4 KiB alignment, the 52-bit physical limit and the current 2 MiB MMIO window limit. Exactly one matching memory descriptor is required.

The loader does not call `SetBarAttributes`, `Attributes(Enable)`, `Map`, or any memory/I/O write method. It does not enable Bus Master or activate DMA.

## Handoff and kernel validation

`swamp_bootinfo` contains a magic, version 1, count (maximum four), and records with segment/BDF, vendor/device, PCI command flags, BAR0 base, width and length. `xhci-fw` rechecks record bounds and cross-checks the current PCI config for segment 0. Records for nonzero PCI segments are skipped until PCIe ECAM support is available.

The resource length is firmware-reported metadata, not cryptographically authenticated. The current kernel only displays it; it does not map the BAR or touch xHCI registers.

## Tests

- `make uefi-pci-resource-host-test`: descriptor parser and mocked UEFI protocol collection under UBSan.
- `make xhci-pci-inventory-host-test`: live PCI configuration cross-check of firmware records under UBSan.
- `make -j4 live-host-test` and `make -j4 boot-host-test`: full completed host groups.

Actual firmware `GetBarAttributes`, QEMU/OVMF and physical USB validation remain pending.

## Specification references

- UEFI Specification 2.10, section 14.4.1 (`EFI_PCI_IO_PROTOCOL`) and section 14.4.18 (`GetBarAttributes`): https://uefi.org/specs/UEFI/2.10/14_Protocols_PCI_Bus_Support.html
- ACPI Specification 6.6, section 6.4.3.5.1 (QWord Address Space Descriptor): https://uefi.org/specs/ACPI/6.6/06_Device_Configuration.html
