# SwampOS roadmap — proposed engineering milestones

Do not treat numbers as estimates of guaranteed completion. The user requested fewer, larger builds, real implementation and no intermediate confirmations. Keep the target 0.20.0 until its acceptance gates are met. Adjust build numbers as required; no artificial caps.

| Proposed checkpoint | Integrated work | Acceptance direction |
|---|---|---|
| 36 | Retained desktop, damage tracking and cursor composition | Host pixel comparisons and QEMU/OVMF PS/2 input passed. VirtualBox and physical display confirmation remain open. |
| 37 | xHCI, dual USB input and VT-d | Q35 guest keyboard/mouse, table inspection, revocation and disconnect passed. Physical platform qualification remains open. |
| 38 | AHCI/NVMe, persistent storage, VFS | Two native Q35 transports passed explicit mount, alternating commits, cold boot, corrupt-record recovery and refusal tests. General file allocation and hardware qualification remain open. |
| 39 | SwampGuard, sandbox, processes, Ring 3 | Enforced per-app permissions, multi-client lifecycle, live revocation, persistent authenticated policy |
| 40 | NIC drivers, TCP/IP, DHCP, DNS, sockets | Guest networking against controlled endpoints; no bypass of user policies |
| 41 | SwampVPN, SwampWall | Real tunnel backend, kill switch, trusted configs, per-app enforcement |
| 42 | SwampLogin, SwampVault, protected boot | Verified chain, separate auth partition, authenticated disk unlock, recovery and negative tests |
| 43 | SWP, SDK/libc, native applications | Real package lifecycle, ABI and app isolation, dependency/licensing checks |
| 44 | Integration and optimization | Clean builds, regressions, performance, hardware/VM acceptance |
| 45+ | Release candidate | Stable Live ISO, rollback, docs, honest scope, unresolved blockers tracked |

## Long-term proposed milestones
0.20.0 The Swamp Awakens — stable Live Desktop. 0.30.0 Iron Onion — hardware. 0.40.0 Deep Roots — storage. 0.50.0 Swamp Fortress — security. 0.60.0 Connected Swamp — network/VPN. 0.70.0 Swamp Ecosystem — package/SDK/apps. 0.80.0 Beyond the Swamp — Linux/Windows ABI compatibility. 0.90.0 Swamp Titan — GPU/compute/optimization. 0.95.0 The Last Onion — installer/recovery. 0.99.0 release candidate. 1.0.0 stable supported release.

Names and numbering are prior assistant proposals, not user-approved release tags. Full original scope is in AGENTS.md. DEB/RPM package parsing is not Linux binary compatibility; PE parsing is not Win32 execution; GPU inventory is not CUDA. For all, define supported hardware/ABI, dependencies and actual acceptance tests. No numeric readiness estimate substitutes for release gates.
