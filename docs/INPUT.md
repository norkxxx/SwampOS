# Farkuad input & boot menu

The 0.13.0 Live ISO booted on a user's VirtualBox VM after enabling EFI. Keyboard/mouse did not work. The previous polling loop preferred COM1, whose absent-port read can return 0xff; this can suppress every PS/2 poll. The old code also depended on firmware leaving the i8042 controller configured. These are plausible causes, not a confirmed guest diagnosis.

## Boot menu

The menu is a native UEFI text application before ExitBootServices, independent of the kernel's PS/2 driver. Arrow keys move the highlighted choice; Enter or digits 1–4 select; Esc returns to firmware. The default is Live Desktop after 100 × 100 ms stalls. Display overhead can extend this slightly. The menu does not unlock encrypted volumes, run GRUB, or modify firmware boot entries.

1. Live Desktop: kernel framebuffer, PS/2 keyboard and relative PS/2 mouse.
2. Classic Desktop keyboard only: original Farkuad tabbed UI, do not activate the auxiliary port.
3. Serial recovery: no desktop mapping, COM1 terminal (requires configured UART).
4. Return to firmware: return from the EFI application.

## Kernel input

Polling only: PIC IRQ1 and IRQ12 remain masked. The driver disables and flushes both i8042 ports, configures translation, enables the keyboard, sets scan set 2 (translated to set 1), enables scanning, and optionally initializes the auxiliary mouse. ACK and RESEND handling are bounded. The shared output buffer is demultiplexed using status bit 5. COM1 is probed with its scratch register and polled after PS/2.

A three-byte relative mouse packet moves a clipped cursor. Left-button transitions now operate the window manager, including dragging/resizing, title controls, taskbar and launcher. Classic mode retains the original tabbed UI. Wheel, USB HID, absolute tablet and native USB keyboards are unsupported. Tab switches tools, Esc returns to terminal. US keyboard layout.

VirtualBox: power off, select Settings > System > Motherboard > Pointing Device > PS/2 Mouse, enable EFI, attach the ISO under Storage. Click inside the VM to capture the mouse; release via the configured Host key. COM1 is optional for desktop and required for recovery. Serial startup messages `PS2_KEYBOARD_ACK` and `PS2_MOUSE_ACK` indicate command acknowledgements, not successful real-device typing.

`make input-host-test` checks mock controller transactions, ACK/resend, absent/stuck hardware, demultiplexing, mouse decoder and UI hit tests, plus a mock UEFI menu. Actual guest and physical input must still be validated.
