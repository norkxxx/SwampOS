# SwampGuard volume identity — Build 15

## Key

A policy entry is identified by `(EXT4 superblock UUID[16], inode number)`. The 128-bit UUID is copied from superblock offset 104 during the strict read-only EXT4 mount. The mount is rejected if the UUID is all zero. The guard table compares all 16 UUID bytes and the inode number.

## Behavior

- Different UUIDs: independent policy entries even when inode numbers are equal.
- Same UUID remount: existing in-memory policy remains attached to the same filesystem identity.
- Hard links: same inode on the same volume share the same ceiling.
- Cloned filesystem with duplicated UUID: intentionally shares the same ceiling. This is conservative; a clone cannot gain a more permissive default by changing the mount path.
- Unrecognized application: default ceiling remains `CAP_ALL`; this is only a ceiling, never a grant. The launcher still has to request capabilities explicitly.
- Zero UUID: mount rejected.

## Limits

The policy table is still volatile and limited to 32 entries. It is not authenticated, signed, or stored on disk. A future persistent design needs a signed policy database, UUID enrollment, duplicate-UUID handling, authenticated user identity and rollback protection. The audit detail currently does not include the full UUID.

## Verification

`tests/volume_guard_host.c` reads the generated EXT4 fixture, creates a second fixture with a different UUID but the same inode numbering, verifies independent ceilings and removal, confirms same-volume remount stability, and checks rejection of an all-zero UUID.
