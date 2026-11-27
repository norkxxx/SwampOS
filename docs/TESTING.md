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
