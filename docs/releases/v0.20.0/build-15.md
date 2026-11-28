# Build 15 — Volume-scoped SwampGuard identities

**Target:** SwampOS 0.20.0 — THE SWAMP AWAKENS

## Changes

- EXT4 mount extracts the full 128-bit filesystem UUID from the superblock.
- Zero UUIDs are rejected by the strict read-only EXT4 profile.
- SwampGuard keys ceilings by `(UUID, inode)`, preventing equal inode numbers on different UUID volumes from sharing policy.
- Same-volume unmount/remount preserves the profile during the current boot session.
- Hard links on one filesystem continue to share policy. Clones with duplicate UUIDs intentionally share policy.

## Validation

- Clean kernel/EFI/ISO build passed.
- Volume identity tests passed with UBSan.
- 100,000 policy-ceiling cases passed.
- Full host regression recipes passed; aggregate command hit the tool time limit during the final vault test, which passed separately.
- ISO structure, EFI payload extraction and 22 artifact checksums passed.
- Guest and physical hardware validation remain pending.

## Limitations

The 32-entry policy table remains volatile and unauthenticated. No persistent signed policy database or UUID enrollment UI exists. Duplicate UUID clones share policy.
