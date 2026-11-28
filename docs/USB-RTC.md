# Hardware groundwork — USB and RTC

## USB boot HID

`drivers/usb/hid/boot_hid.c` parses a bounded configuration descriptor subset (maximum 4096 bytes, eight selected alternate-zero boot interfaces). It extracts a single interrupt-IN endpoint per recognized boot keyboard or mouse interface. Unknown non-interface/endpoint descriptors are skipped; this is not a full USB descriptor validator or report descriptor interpreter. The keyboard codec accepts exact 8-byte boot reports, tracks pressed keys, handles Shift/Caps Lock and rejects rollover/error reports and duplicate nonzero usages; it maps US ASCII and up/down arrows, suppressing Ctrl/Alt/GUI combinations. Mouse accepts exact three-byte boot reports with three buttons and signed relative movement; no wheel, report IDs, absolute tablet, NKRO, hotplug or transport. Codecs are not connected to the active input loop. `usbtest` invokes synthetic fixtures in the kernel; `usb` performs PCI configuration-space enumeration only. Existing PS/2 path is unchanged.

## RTC

`rtc` explicitly reads the legacy MC146818-compatible CMOS register interface using index port 0x70 and data port 0x71. It never writes CMOS data; selecting the index uses NMI-enabled addressing. The current uniprocessor kernel serializes the operation against maskable interrupts. Bounded polling rejects invalid battery status, update-in-progress, inconsistent snapshots, SET mode and invalid time fields. Supports BCD and binary, 12 and 24 hours. Year is deliberately two digits: century and RTC timezone are unknown. No RTC alarm, time setting, ACPI century register or OS wall clock integration. This is a legacy PC path; real hardware I/O remains unverified.

## Acceptance

`make usb-rtc-host-test` executes the parser and clock logic with mocked input. A physical USB driver still requires controller ownership, MMIO mappings, DMA isolation, transfer rings, enumeration and report delivery, followed by device-level tests. No claim of working USB HID is made.
