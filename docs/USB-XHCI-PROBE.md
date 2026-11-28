# Read-only xHCI capability probe — Build 34

## Command

Run `xhci-probe` from SwampTerminal. The command requires UEFI boot, a version-1 xHCI resource handoff, a complete physical-range map, a segment-0 record, live PCI revalidation and PCI Memory Space already enabled.

The kernel maps the firmware-reported BAR0 aperture through `vm_map_mmio`. The mapping is UC, NX and supervisor-only. The mapper rejects low physical addresses, unaligned/oversized ranges, framebuffer overlap, any overlap with active non-MMIO physical memory, incomplete physical maps, unsupported PAT and allocation failure. Repeating the same mapping is idempotent; a different second MMIO window is rejected.

The command invokes `xhci_caps_read` with a read-only `read32` callback and no `write32` callback. It reads capability registers only: CAPLENGTH, HCIVERSION, HCSPARAMS1/2, HCCPARAMS1, DBOFF and RTSOFF. Register offsets and spans are checked against the BAR aperture.

## Not performed

- No operational-register writes.
- No controller reset or start.
- No BIOS/OS ownership semaphore changes.
- No PCI command-register changes.
- No Bus Master enable.
- No DMA or transfer ring activation.

## Physical memory map

The UEFI handoff carries a second map for physical-range alias checks. It contains all non-MMIO UEFI descriptors (including high RAM, loader/runtime regions, ACPI reclaim/NVS and persistent memory); UEFI MMIO and port-space descriptors are excluded. The low conventional-memory map used by PMM is unchanged. If more than 64 ranges are needed, the map is marked incomplete and MMIO mapping fails closed.

## Tests

- `make mmio-resource-host-test`
- `make uefi-handoff-host-test`
- `make xhci-caps-probe-host-test`

All three use host mocks/UBSan. Actual UEFI firmware and physical controller probing remain unvalidated.
