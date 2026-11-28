# Security boundaries and release gate

## Implemented code

- BIOS E820 bounded collection (64 entries); allocator reserves low memory through the kernel image, excludes nonusable overlaps and partially covered pages, caps managed memory at 128 MiB.
- 4 KiB paging, CR0.WP inherited from bootstrap, EFER.NXE required; null page absent. Supervisor mappings are shared across process roots but unavailable at CPL3. User code is RX, stack RW/NX, guard absent. The trusted kernel's physical alias is writable/NX, so this is not physical-frame-wide W^X.
- Fresh code/stack/table frames per run, scrub on allocation and release; bounded pointer validation and default-deny console grants.
- TSS, ring transition, exception dispatch, watchdog, and interrupt-gated syscall interface.
- Startup stops on VM initialization/self-test, user-test or original core-test failures. `BOOT_OK` is only emitted after all guest tests pass.

## Not verified or not implemented

QEMU and physical hardware execution are unverified in this release. Host simulation checks page-table contents, not CPU enforcement. A constrained external ELF loader is now present; no concurrency between user processes, preemptive process scheduler, SMP, FPU state management, hardened kernel stacks/IST, SMEP/SMAP, KASLR, DMA isolation, side-channel mitigation or general VFS exists. The physical map includes low memory holes, which are excluded from allocation but remain supervisor-mapped. Legacy BIOS bootstrap trusts its disk and firmware and is not secure boot. No production security claims.

The target still requires mandatory per-app sandboxing, mediated device/network access, signed package and update pipelines, UEFI verified boot, separate signed SwampLogin partition, authenticated user-driven key release, encrypted EXT4 volumes, TPM-backed key handling, secure recovery and rollback protection. RSA wraps keys, AES encrypts blocks; never invent cryptographic primitives. An offline authenticated image envelope is implemented in host tooling only; no kernel block encryption or verified preboot authentication is implemented. CUDA support requires licensed proprietary components and compatible drivers/guest isolation.

## Release gate

`make test` is host/static. `make runtime` must pass before treating Ring 3, IRQs, NX or watchdog as demonstrated on a virtual CPU. Physical hardware and adversarial security testing are separate gates. Do not install on a real disk or run untrusted binaries in this experimental build.

## Vault additions
See [storage constraints](STORAGE.md) and [offline encryption prototype](VAULT.md). No auto-execution from disk. Console grants are explicit shell choices; disk metadata is not authenticated. No untrusted binary execution until guest enforcement and independent security review are complete.

## SwampBoot boundary (0.7.0)
The lab boot manager is unsigned and deliberately allows Secure Boot disabled. The strict unsigned binary checks firmware state but is not a trusted enrolled boot chain. SwampLogin fails closed, accepts no password and unlocks nothing. GPT checksums are not signatures. Offline registry signatures are meaningful only with an independently trusted public key and rollback floor; they are not yet consulted by firmware. No default private key, password or ciphertext is shipped. Offline provisioning validates envelope shape, not its authenticity; actual authentication occurs only in the host unlock tool. Never use these images for sensitive data.

## Hardware Awakening (0.8.0)

The additional `KernelLab-GPT.img` is a **separate unsigned plaintext development image**, not the protected boot path, and must not be trusted/enrolled. It explicitly requires disabled Secure Boot and contains no encrypted volumes; SwampLogin still accepts no credentials, releases no keys, and never invokes the lab loader. GOP geometry is read-only metadata; keyboard input is for the unprotected lab shell, not password collection. Firmware map conversion excludes all non-conventional and runtime descriptors, checks overlaps, and reserves 1–4 MiB. Physical MMIO remains outside the allocator; read-only ACPI diagnostics can inspect only the existing low physical window. PCI inspection does not size/probe BARs by writes, enable bus mastering, bind a driver, or program DMA. No IOMMU enforcement or APIC/SMP implementation exists.

The lab VFS offers only one read-only mount and explicitly rejects non-Linux filesystem GPT type GUIDs including the SwampVault GUID; mounting does not establish authenticity. The network engine operates only on supplied memory buffers, no NIC or sockets, rejects unsupported/malformed traffic, and is not exposed to Ring 3. VM launch commands disable QEMU network devices by default. `BOOT_OK` now additionally requires the offline network self-test. QEMU and OVMF must demonstrate all guest gates before further security claims. No production trust or physical hardware support is asserted.

## Connectivity (0.9.0)

The previous offline-only claim is superseded solely by the explicit `netlab` trusted-emulator path. It configures one transitional VirtIO adapter, with static DMA buffers and bounded polling, then attempts reset and disables bus mastering. No IOMMU: an untrusted device is not contained. Guest integration is unverified. Default launch targets remain offline. No Ring 3 networking, policy-to-socket wiring, VPN backend, or actual kill-switch enforcement. The SwampWall/VPN policy models are independently tested primitives, not an installed security boundary. Do not enroll unsigned lab boot code or store secrets in the images. Separate SwampLogin remains locked and unchanged.

## Ecosystem (0.10.0)

Host packages use one explicit trusted Ed25519 public key, signed canonical manifest and exact SHA-256/size checks; the store performs no extraction or execution and requests are not capability grants. Host store integrity assumes a trusted local account and parent path; no anti-rollback against a malicious store writer. Foreign format inspection is structural only. The new C example runs via the existing limited console-only ABI. The volatile audit ring records denials/faults/exits but is not a tamper-proof security log. The separate unsigned GuiLab requires disabled Secure Boot and does not authenticate or unlock anything. It must not be trusted/enrolled. No change to encryption, login, hardware isolation, or native networking guarantees.

## Integration (0.11.0)
The operator can grant a bounded copy of `/welcome.txt` and console output to one trusted native ELF run. No arbitrary filesystem syscall exists. Only writable user stack addresses are accepted for resource reads; session state is wiped on return. Run IDs are ephemeral and not security principals across boots. Signed staging happens on the host; the guest trusts the public fixture and rechecks the narrow ELF format, not a package signature. This is not a mandatory general-purpose sandbox or verified boot.

## Overlord (0.12.0)
The earlier no-concurrency statements describe earlier releases. Bounded user groups now switch context on timer/yield and support capability-checked IPC and scratch. Actual CPU enforcement remains unverified. Shared supervisor mapping, no DMA isolation, no XSAVE/SMP, no hardened IST/SMEP/SMAP and no production security guarantee remain. Native pre-boot authentication and encrypted block I/O remain absent. See [OVERLORD](OVERLORD.md).
