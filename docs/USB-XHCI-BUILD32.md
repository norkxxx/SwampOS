# Build 32 — Runtime xHCI PCI Inventory

## Runtime command

Run `xhci` in SwampTerminal. The kernel enumerates PCI functions using the existing read-only PCI configuration mechanism and selects class `0C:03:30` (xHCI). For each candidate it re-reads and validates vendor/device ID, class code and endpoint header, then decodes BAR0.

Output fields:

- `BDF`: PCI bus/device/function.
- `id`: vendor and device identifiers.
- `BAR0`: assigned 32-bit or 64-bit memory base, or `unassigned`.
- `MEM`: PCI Memory Space Enable.
- `MASTER`: PCI Bus Master Enable.
- `IO`: PCI I/O Space Enable.

## Safety boundary

This command is inventory only. It never writes PCI configuration space, never sizes BARs by writing all ones, never enables memory decoding or bus mastering, never maps the BAR and never accesses controller registers. The BAR aperture length is not inferred from the address; a separate trusted resource manager must supply that length before MMIO mapping.

## Tests

`make xhci-pci-inventory-host-test` covers 32-bit and 64-bit BAR decoding, unassigned BARs, PCI Command flags, mismatched class/identity/header, I/O BAR rejection, reserved BAR type rejection and physical-address-width rejection under UBSan.

## Remaining work

ACPI/firmware resource reconciliation, PCIe ECAM support where needed, validated BAR aperture acquisition, firmware ownership, controlled PCI command programming, IOMMU policy, real MMIO mapping, controller initialization and guest/physical validation remain outstanding.
