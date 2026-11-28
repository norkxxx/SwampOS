# Security boundaries and release gate

## Implemented code

- BIOS E820 bounded collection (64 entries); allocator reserves low memory through the kernel image, excludes nonusable overlaps and partially covered pages, caps managed memory at 128 MiB.
- 4 KiB paging, CR0.WP inherited from bootstrap, EFER.NXE required; null page absent. Supervisor mappings are shared across process roots but unavailable at CPL3. User code is RX, stack RW/NX, guard absent. The trusted kernel's physical alias is writable/NX, so this is not physical-frame-wide W^X.
- Fresh code/stack/table frames per run, scrub on allocation and release; bounded pointer validation and default-deny console grants.
- TSS, ring transition, exception dispatch, watchdog, and interrupt-gated syscall interface.
- Startup stops on VM initialization/self-test, user-test or original core-test failures. `BOOT_OK` is only emitted after all guest tests pass.

## Not verified or not implemented

QEMU and physical hardware execution are unverified in this release. Host simulation checks page-table contents, not CPU enforcement. No external program loader, concurrency between user processes, preemptive process scheduler, SMP, FPU state management, hardened kernel stacks/IST, SMEP/SMAP, KASLR, DMA isolation, side-channel mitigation or general VFS exists. The physical map includes low memory holes, which are excluded from allocation but remain supervisor-mapped. Legacy BIOS bootstrap trusts its disk and firmware and is not secure boot. No production security claims.

The target still requires mandatory per-app sandboxing, mediated device/network access, signed package and update pipelines, UEFI verified boot, separate signed SwampLogin partition, authenticated user-driven key release, encrypted EXT4 volumes, TPM-backed key handling, secure recovery and rollback protection. RSA wraps keys, AES encrypts blocks; never invent cryptographic primitives. Encryption and integrity formats are not yet implemented. CUDA support requires licensed proprietary components and compatible drivers/guest isolation.

## Release gate

`make test` is host/static. `make runtime` must pass before treating Ring 3, IRQs, NX or watchdog as demonstrated on a virtual CPU. Physical hardware and adversarial security testing are separate gates. Do not install on a real disk or run untrusted binaries in this experimental build.
