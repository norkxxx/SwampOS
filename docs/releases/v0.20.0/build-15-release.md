# THE SWAMP AWAKENS — Build 15

**SwampOS target 0.20.0 / internal development checkpoint**

> ITS YOUR SWAMP - ITS YOUR LIFE

SwampGuard launch ceilings are now keyed by the full 128-bit EXT4 filesystem UUID plus inode number. This prevents an application inode on a distinct volume with a different UUID from inheriting another volume's policy. The same volume retains its policy across unmount/remount. Zero-UUID filesystems are rejected by the narrow EXT4 profile.

The hybrid UEFI Live ISO was rebuilt from the updated kernel. Host build and structural ISO checks passed. The full parallel host regression reached the final vault test before the tool time limit; that cryptographic test passed separately. QEMU/VirtualBox and physical hardware execution were not available. This is not the final 0.20.0 release.
