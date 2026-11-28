# Reproduce tests

```sh
./install.sh
make clean
make test
make runtime
make run
```

`make test` compiles the freestanding kernel and validates raw image geometry, ELF layout and symbols, then runs the actual cooperative context-switch routine in a native host process; PMM, capability and VM table tests run with UBSan. VM simulation uses a host RAM array; it does not execute privileged instructions. `make runtime` fails if QEMU is unavailable, rather than silently skipping. GitHub Actions runs both gates and uploads the serial log even on a failed guest test (when a log exists).

Expected integration markers (not a captured boot): `VM_TEST_OK`, nine `[user] ... PASS` lines, `USER_TEST_OK`, `CORE_TEST_OK`, `BOOT_OK`, `swamp> `. The integration test sends interactive commands and requires the expected results. Host PASS does not imply guest PASS.

## Vault gates
`make test` includes storage parser tests (40,000 deterministic mutations per run) and offline RSA/AES authentication checks. `make runtime` adds secondary IDE fixture, ls/cat, and external ELF execution with and without console grants. No guest results are inferred from the host tests.

## Hardware Awakening gates
`make hardware-host-test` tests ACPI, PCI BAR/capability parsing, keyboard state, EFI map conversion, offline frame processing, real GPT-backed EXT4 mount, PE relocation presence and FAT32 extraction of the exact embedded kernel. `make test` includes this suite with the prior regression suite. The three guest images require four runtime gates: `make runtime`, `make uefi-runtime`, `make kernel-lab-runtime`, `make storage-lab-runtime`. Firmware tests use a fresh writable copy of a matching non-Secure-Boot OVMF variables template; no firmware keys are enrolled. The expected boot transcript contains `NET_TEST_OK` as well as the existing markers. Runtime tests are distinct from host tests and missing QEMU/OVMF is reported as NOT RUN.
