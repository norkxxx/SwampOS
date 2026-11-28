# Intel VT-d DMAR inventory — Build 35

## Command

Run `iommu` in SwampTerminal. The command uses the RSDP already handed off by UEFI or scans the legacy BIOS RSDP windows, then validates the ACPI root and DMAR table checksums. The current physical reader is limited to the mapped low 128 MiB window; ACPI tables above that window are not accessible yet.

Supported DMAR structures: DRHD (remapping hardware units), RMRR (reserved memory regions), ATSR and SATC device scopes, RHSA length validation, and ANDD string termination. Device scopes are bounded to 64 records; DRHD units are bounded to eight. Malformed structures or inventory overflow mark DMAR incomplete and suppress coverage claims.

The command reports host address width, DRHD register bases, PCI segment, include-all flags, RMRR count, scope count and interrupt-remapping advertisement. For each xHCI firmware record, the kernel first revalidates the current PCI function/BAR/command state. Coverage is considered proven only when a matching-segment DRHD has INCLUDE_PCI_ALL or an exact one-hop PCI endpoint scope matching bus/device/function. Multi-hop PCI paths and sub-hierarchy scopes are not treated as proven. Scope/DRHD inventory overflow also suppresses all coverage claims.

## Safety boundary

- DMAR register bases are displayed only; no IOMMU MMIO access occurs.
- No root/context/interrupt-remapping tables are allocated.
- No PCI attributes or Bus Master state are changed.
- No DMA is activated.
- AMD IVRS parsing is not implemented.

## Tests

`make acpi-dmar-host-test` builds synthetic RSDP/XSDT/DMAR tables and checks checksums, DRHD/RMRR/ATSR, include-all, direct endpoint scopes, segment/BDF matching, truncation and malformed descriptors under UBSan. Actual firmware and physical IOMMU validation remain pending.
