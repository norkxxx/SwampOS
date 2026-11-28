# Development history (condensed, not a chat transcript)

Early releases: 0.1 Genesis, 0.2 Onion, 0.3 Deep Onion, 0.4 Donkey, 0.5 SwampBox, 0.6 Vault, 0.7 SwampBoot, 0.8 Hardware Awakening, 0.9 Connectivity, 0.10 Ecosystem, 0.11 Integration, 0.12 Overlord, 0.13 Farkuad (including ISO revision), 0.13.1 Farkuad Take Control (user-confirmed VirtualBox fallback).

Build 01–38 are cumulative development checkpoints for target 0.20.0, not separate public versions. For exact implementation and evidence read existing HANDOFF.md, FEATURES.md, CHANGELOG.md, RELEASE.md, TEST-REPORT.md and build/logs. Past host PASS does not establish guest or physical PASS.

| Build | Focus |
|---|---|
| 01 | Window Manager |
| 02 | Writable RAMFS |
| 03 | Kernel Expansion & Terminal |
| 04 | Desktop Applications |
| 05 | Hierarchical RAMFS |
| 06 | Native GUI Client |
| 07 | Hardware Groundwork |
| 08 | SwampFS Expansion |
| 09 | ATA Storage Driver |
| 10 | Onion Resource Security |
| 11 | Native Resource Streaming |
| 12 | SwampGuard Security |
| 13 | SwampGuard Policy Editor |
| 14 | Asynchronous Native GUI |
| 15 | Volume Identity & SwampGuard |
| 16 | xHCI Ring Transport |
| 17 | xHCI Controller Lifecycle |
| 18 | USB Device Enumeration |
| 19 | xHCI EP0 Control Transfers |
| 20 | Integrated USB EP0 Transport |
| 21 | Device Context & Slot Lifecycle |
| 22 | USB2 Root Port Management |
| 23 | xHCI Interrupt Endpoint Contexts |
| 24 | xHCI Interrupt Transfer Ring |
| 25 | Nonblocking HID Transport |
| 26 | Full-Speed EP0 Support |
| 27 | Integrated USB2 HID Session |
| 28 | Pinned DMA Memory |
| 29 | PCI BAR & UC MMIO |
| 30 | BIOS/OS Firmware Ownership |
| 31 | Dynamic Live Image & Build System |
| 32 | Runtime xHCI PCI Discovery |
| 33 | UEFI PCI Resource Handoff |
| 34 | Read-only xHCI Capability Probe |
| 35 | Intel VT-d / DMAR Inventory |
| 36 | Retained desktop, cursor damage and guest input validation |
| 37 | Native USB keyboard and mouse with per-device VT-d translation on Q35 |
| 38 | AHCI and NVMe, persistent note and cold-boot recovery |

Build 31 archive packaging was corrected to restore missing historical logs. Build 38 is the current source checkpoint. The Build 35 migration materials remain the historical baseline.
