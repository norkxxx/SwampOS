# Explicit resource grants — Builds 10–11

The trusted kernel shell can launch a native ELF with exactly one selected data resource. `run-resource APP ram:RELATIVE_PATH` and `run-resource APP ext:/ABSOLUTE_PATH` grant CAP_DATA only. `run-resource-console` additionally grants CAP_CONSOLE. Old `run-data` retains its compatibility behavior. The caller does not gain access to all storage and cannot choose a new resource through a syscall. The existing session ABI is unchanged (`swamp_data_read`).

The executable is read from the existing mounted read-only EXT4. The resource is a regular file in the existing RAMFS or the mounted EXT4 namespace; no physical drive is opened by the grant machinery. Paths are limited to eight components, each at most 31 printable ASCII characters; no dot/dot-dot components, repeated or trailing separators, colon or backslash. RAM paths are relative; EXT4 paths absolute. Spaces in names are not supported. The complete file must fit within 64 KiB (8 KiB for RAMFS files); oversized files are rejected, not truncated. Empty files are accepted. Source data is copied into kernel staging before launch, the session owns its own copy, and both are cleared on return; failure clears the staging snapshot. Staging and session buffers are statically allocated in supervisor-only kernel memory, not on the limited kernel stack. Each syscall remains limited to 256 bytes. The serialized shell does not expose a concurrent mutation window. The application cannot access other files. Data is intentionally readable by the granted application, and output can disclose it if console permission is also granted.

Examples:

```sh
ramwrite demo.txt Hello_SwampOS
run-resource-console /bin/integration-demo ram:demo.txt
run-resource-console /bin/integration-demo ext:/welcome.txt
run-resource /bin/integration-demo ram:demo.txt
```

The last example should exit 25 because the existing integration demo expects console access. A normal `run` still defaults to no capabilities. The demo reads up to 128 bytes, so use a short nonempty resource for its success case. `make resource-host-test` checks the actual snapshot builder and session API using real EXT4 and RAMFS, malformed paths, sizes, immutability and scrubbing, with 50,000 lifecycle cycles. Guest assertions are included in `tests/live_boot.py` but have not been executed. This is a per-launch grant, not an installed application identity database or complete permission manager.

## Build 11

`/bin/resource-scan` reads the whole resource using offset-based chunks and emits the count and FNV-1a checksum. Run with both CAP_DATA and CAP_CONSOLE. A missing grant returns exit 13 or 25. `run-data` retains its original 256-byte compatibility resource limit. There is no new syscall, writeback, cross-process resource sharing or persistent permission database. Tests cover 10 boundary sizes (including zero and 65,536 bytes), overlong requests, EOF and overflow offsets, actual dispatcher checks under mock CPU, and executable extraction from the EXT4 fixture.
