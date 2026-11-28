# Native SDK v0

The existing native ABI now has a C header, `sdk/include/swamp.h`. Exit, console write (explicit grant), capability query, per-run ID and bounded assigned-resource read are available. No libc, allocator, threads, general file access, networking or device syscalls. The loader accepts the narrow static ELF64 profile in STORAGE.md: single RX PT_LOAD at 0x40000000, at most 4 KiB, stack elsewhere, no dynamic relocation or global mutable data. Compiler uses GPR-only freestanding x86_64 code. The C example `userspace/sdk_demo.c` produces `build/sdk-demo.elf` and is included in the unencrypted demo EXT4 volume. Actual Ring 3 execution still needs QEMU.

```sh
make build/sdk-demo.elf
make run
# in the SwampShell (expected behavior)
run /bin/sdk-demo          # status 13, no output permission
run-console /bin/sdk-demo  # SWAMP_SDK_HELLO, status 0
```

Use a trusted local development toolchain. This SDK does not permit loading arbitrary Linux ELF executables or Windows binaries. SDK will evolve with the ABI; no compatibility stability promise.

## Integration example
`make build/integration-demo.elf` builds the new demo. `run-data /bin/integration-demo` grants console and a read-only welcome-file snapshot; `run` and `run-console` deny data access. `swamp_id()` and `swamp_data_read(buf, n, offset)` wrap the new calls.

## Overlord SDK extension
New inline wrappers: swamp_yield, swamp_send, swamp_recv, swamp_role, swamp_scratch_write, swamp_scratch_read. `userspace/app-data.ld` supports initialized global data and BSS; `make build/overlord-demo.elf` builds the sample. See [OVERLORD](OVERLORD.md).
