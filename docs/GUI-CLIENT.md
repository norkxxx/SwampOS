# Native GUI capture ABI — Build 06

The new example is built from `userspace/gui_demo.c` with the freestanding SDK and stored as `/bin/gui-demo` on the read-only Live EXT4 volume. It is not linked into the kernel. `CAP_GUI` (bit 5) is available only in the kernel's explicit single-image capture path; the standard `run` and group execution paths do not grant it. Syscall 11 accepts exactly 128 bytes, representing a 16×8 image of indexed colors 0–5. The pointer must be within one valid readable user mapping. The kernel checks the palette before staging a copy, exposes no framebuffer address, and publishes the last valid staged frame to the caller only if the process exits with status zero. Nonzero exit, exception, watchdog or absent frame yields no published image. The staging buffer is scrubbed at the end of each run.

The shell command `gui-demo` tests the capture and reports status. Paint Shift+G requests the same fixed demonstration ELF and scales the captured 16×8 image to its 24×12 canvas. Regular `run /bin/gui-demo` should return 13 because the application refuses to proceed without the capability. No console, filesystem, network or device capability is automatically granted.

The host harness runs the real C trap dispatcher with mock CPU entry and CR3 changes. It exercises capability rejection, length, pointer and palette checks, clean exit, nonzero exit, exception and stale-frame rejection over 2,000 sessions. The ELF is read back from EXT4 and compared with the separately built binary. These tests do not execute guest CPU instructions; QEMU/VirtualBox acceptance remains mandatory. This is a synchronous, one-shot ABI, not a general window protocol, secure desktop or per-app GUI event loop.

## Build 14 update

The synchronous capture remains available. The Paint shortcut now starts a resumable client, see [GUI-ASYNC.md](GUI-ASYNC.md).
