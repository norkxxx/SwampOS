# Integration 0.11.0

The native loader still runs one application at a time. `session_begin` copies at most 256 bytes from an operator-selected built-in resource into kernel-owned memory. The shell's `run-data` command snapshots only `/welcome.txt` from the mounted public read-only EXT4 volume. It grants both console and data access; `run` grants neither and `run-console` grants only console. No privilege is derived from a package manifest.

The `int 0x80` handler checks CAP_DATA and a writable stack range before copying. The read uses RDX as an unsigned byte offset and clips at EOF. The session is cleared when `user_enter` returns, including normal exit, fault and watchdog paths. No persistent PID or concurrent execution is provided.

## Verified staging (host)

`tools/stage_native.py STORE PUBLIC_KEY PACKAGE bin/MEMBER OUTPUT` reads from the active verified SWP snapshot, checks the structural native profile, and exclusively creates OUTPUT. It neither executes a binary nor grants permissions. The guest ELF loader is authoritative for its own narrower constraints. To build a dedicated public test image using a verified integration demo export, run:

```sh
./build/mkvolume staging.ext4 build/hello.elf build/sdk-demo.elf exported-demo.elf
```

The host integration test signs a temporary package, stages the demo, and checks that the produced EXT4 image is identical to the normal build. No test signing keys are distributed.

## Runtime expectations (unverified)

`run /bin/integration-demo` and `run-console /bin/integration-demo` should return 13. `run-data /bin/integration-demo` should return 0 after reading the assigned file. BIOS QEMU gate checks these outcomes. No guest CPU or independent security audit has been completed.
