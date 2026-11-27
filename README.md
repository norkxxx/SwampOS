# SwampOS 0.1.0 Genesis — bootstrap
Security has layers.

Experimental bare-metal BIOS/i386 bootstrap, not a secure production OS. The final target is x86_64 UEFI with a Rust/C/ASM kernel. This build uses C/ASM because the build environment lacks Rust and QEMU.

Build on Debian/Ubuntu or WSL: `./install.sh && make test`. Run `make run` and type `help` in the serial terminal. The VGA window mirrors output; keyboard input is serial only. Use a disposable VM, not a physical disk. The raw disk image is not an ISO. No runtime boot verification has been performed.

See docs/ARCHITECTURE.md for the retained requirements. No unimplemented security feature is claimed active.
