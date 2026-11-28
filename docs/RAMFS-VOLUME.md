# Expanded volatile filesystem

Build 08 raises the bounded kernel-owned RAM store to 64 files, each up to 8192 bytes, and 64 directory slots (512 KiB aggregate payload, excluding metadata). This is a fixed BSS reservation, not an allocator-backed filesystem. Maximum canonical relative path: 95 bytes, 8 components, 31 bytes each. The serialized desktop/shell execution model is unchanged.

- `ramappend PATH TEXT`: create or append, reject oversized results without modifying existing data.
- `rammv DIR NEW_DIR`: preflight the destination, every descendant path and the parent; reject self-descendant moves, collisions and excessive depth before mutation. Atomic with respect to the serialized shell, not a crash-durable transaction.
- `ramfs_read_at`: bounded internal range reads, returns the available bytes up to the supplied capacity; no new user syscall.
- `ramimport /SOURCE DEST`: copy a regular file from the current mounted read-only EXT4 into RAMFS. Explicitly replaces an existing RAM file after successful source read; rejects files exceeding 8192 bytes, invalid names, directories and unavailable volumes. The source device is read-only. Scratch bytes are cleared after the read attempt.

Example: `ramimport /welcome.txt welcome.txt`. The 9000-byte `/pattern.bin` is deliberately larger than the current file limit and will be rejected. Data is volatile. No automatic mounting of physical disks and no internal disk writes are introduced. There is no filesystem permission ABI or durable journaling yet.

Run `make ramfs-volume-host-test`. The test uses the real built EXT4 image and the kernel reader, validates 40,000 directory moves and verifies the entire RAM store remains unchanged on rejected moves. Old RAMFS and desktop regression tests continue to run.
