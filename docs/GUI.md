# SwampGUI — graphics foundation

`gui/surface.c` is a shared software renderer with clipped rectangles and original bitmap glyphs. `swamp_desktop()` draws a 640×360 static English dark-green scene. Host testing renders the actual scene to `build/gui-preview.ppm`; the PNG in `assets/` is derived from these pixels, not a guest screenshot.

`GUILAB.EFI` is an independent, unsigned UEFI graphics application. It explicitly requires disabled Secure Boot, requests GOP, validates geometry, draws into an internal 640×360 BGR buffer, and invokes firmware `Blt(BufferToVideo)` in the center of the display. It accepts no credentials, accesses no disk, never calls ExitBootServices and never loads the kernel. It stays in the preview until the firmware watchdog/reset. `GuiLab-GPT.img` contains this lab, not a desktop OS. Do not enroll it in Secure Boot or use it as SwampLogin.

`make gui-lab-runtime` compares the actual guest screenshot against host reference pixels via QMP; QEMU + OVMF required. `make gui-lab` builds the image only. No kernel framebuffer mapping, compositing, window management, mouse, GPU acceleration, clipboard or user application UI yet. Those remain architecture targets.
