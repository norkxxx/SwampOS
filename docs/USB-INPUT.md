# USB input on Q35

Boot the native x64 UEFI ISO from the project root with `make live-usb-desktop`. This starts a disposable Q35 guest with 128 MiB of RAM, a fresh OVMF variable store, optical boot, Intel VT-d, one xHCI controller and two directly attached USB HID devices. The keyboard and mouse share the controller event ring. The terminal remains attached to COM1. The Live root is the embedded EXT4 volume in RAM.

## Activation and control

The Live desktop checks for the emulated Q35 host bridge and the single-byte QEMU firmware configuration file `opt/swamp/usb-input` containing `1`. The supplied launcher sets this flag and disables the i8042 controller. Ordinary Live boots retain the existing PS/2 path. Classic and serial recovery keep their prior input settings.

| Terminal command | Behaviour |
|---|---|
| `usb-status` | Show session state and the cumulative count of decoded HID reports |
| `usb-stop` | Halt xHCI, disable PCI bus mastering, revoke the VT-d context, invalidate caches and release pinned buffers after confirmation |
| `usb-start` | Re-enumerate attached devices in the opted-in guest. Use COM1 or a remaining input source after stopping USB. |
| `xhci`, `iommu` | Inspect PCI identity and ACPI inventory. These commands describe hardware discovery. |

A disconnect ends the shared session. Reconnect the device and issue `usb-start` through COM1. If quiescence or revocation cannot be confirmed, the kernel retains the buffers and reports quarantine until reboot.

## Supported DMA configuration

This implementation owns one previously idle legacy Intel VT-d unit on an explicitly selected Q35 guest. It checks the firmware BAR against live PCI configuration and requires unambiguous DMAR coverage, a single DRHD, zero RMRRs and complete physical ranges. The CPU accesses controller and IOMMU registers through separate supervisor-only uncached mappings. The kernel publishes page tables with explicit cache-line flushes when page walks require them.

The domain maps one PCI requester to pinned pages using 4 KiB translations. The controller ring/DCBAA, device contexts, control transfers and interrupt transfers occupy separate IOVA windows. A pair occupies seven windows with twelve mapped pages. Translation tables and unrelated memory have no leaf mappings. The guest audit inspects the actual root, context and all three second-level tables through QEMU's physical monitor.

One or two directly attached USB 2.0 boot keyboards or mice are supported. One boot HID interface per device, a relative three-byte mouse prefix and US keyboard layout are implemented. Hubs, USB storage transport in the kernel, tablets, physical platform activation, AMD IOMMU and general PCI DMA management remain open. The optical test profile loads its disk image before kernel xHCI takeover.
