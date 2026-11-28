# SwampGuard — volume-scoped launch capability ceilings

Build 15 keys a policy by the full 128-bit EXT4 superblock UUID plus inode number, not by inode or pathname alone. Two hard-link names for one inode on the same filesystem share a ceiling. The same inode number on two filesystems with different UUIDs receives independent ceilings. A remount of the same UUID retains the policy during the current boot session.

The table contains up to 32 entries and is volatile. It is not a cryptographic application identity or a persistent policy database. Cloned filesystems that retain the same UUID intentionally share policy; this avoids a clone silently escaping a restrictive profile. The current build does not enroll, regenerate or authenticate filesystem UUIDs.

| Permission | Bit value |
|---|---:|
| Console | 1 |
| Selected data | 2 |
| IPC | 4 |
| Scratch | 8 |
| GUI capture | 32 |

Combine permitted bits by addition. 0 forbids all optional capabilities; 47 is the unrestricted ceiling. Reserved bit 16 and unknown bits are rejected. The ceiling never grants a capability; a launch must explicitly request it and all requested bits must be within the ceiling. Rejection occurs before process creation or selected-resource copy. Group launch rejects the entire group if any member is denied. A launch without optional capabilities is still allowed under ceiling zero.

```sh
guard-set /bin/resource-scan 2
guard-show /bin/resource-scan
run-resource-console /bin/resource-scan ext:/pattern.bin
# denied (console is outside ceiling)
guard-set /bin/resource-scan 3
run-resource-console /bin/resource-scan ext:/pattern.bin
guard-list
guard-reset /bin/resource-scan
```

Only trusted shell commands and the bundled graphical editor can edit the table. `guard-reset` restores the default ceiling, not an automatic grant. Audit event 5 is denied launch and event 6 is an accepted edit; the current event detail still records inode and requested/ceiling bits, not the full UUID.

Restrictions survive unmount/remount of the same EXT4 UUID during the session. They do not survive reboot. No live revocation for synchronous clients, per-user login, persistent signed profiles, network/USB permissions, or arbitrary-application GUI policy editor is added. Built-in kernel tools and internal self-tests are trusted, outside ELF launch policy. Host tests exercise actual storage command dispatch with real EXT4 and mocked CPU entry; guest boot remains pending.
