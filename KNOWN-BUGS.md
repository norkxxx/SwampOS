# Known bugs and validation debt — Build 38

The storage profile is limited to one fixed file, one controller and an exclusive VT-d domain. Physical devices, VirtualBox storage and concurrent xHCI plus storage remain unverified.

## Build 37

The USB profile currently requires Q35, optical boot, a single idle legacy VT-d unit and up to two directly attached USB2 HID devices. Port disconnect ends the session for both devices. COM1 remains available for restart. VirtualBox PS/2 display confirmation and physical USB qualification are pending.

## Build 36 display record

The blank intermediate redraw was removed from the desktop path. Build 36 guest checks at 1280 × 800 compared the complete PS/2 frames with the expected pixels during movement, drag and resize. VirtualBox user confirmation and hardware scanout timing remain open. The QEMU guest also completed the native GUI resume test. The full host regression passed.

## Build 35 investigation (historical)

### P0: Screen flicker while moving mouse (reported by user)
Source inspection: `desktop/window.c` mouse handling sets `s->dirty=1` even for ordinary movement. `wm_render()` clears the entire visible surface with `surface_rect(s,0,0,...)` before drawing the background/windows/taskbar/cursor. `kernel/desktop.c` calls `wm_render(&fb,...)` or `live_render(&fb,...)` directly against the visible framebuffer whenever dirty. This exposes intermediate redraws; it is a strong code-based explanation for the reported flicker, not a reproduced VM diagnosis.

Build 36 acceptance:
- Render complete frames into validated supervisor-owned back buffer. Account for pitch, pixel format, dimensions, overflow, allocation bounds and failure paths. Keep Classic mode working.
- Add damage rectangles (old/new cursor bounds, window movement, resize, open/close, z-order, theme changes), clip and coalesce; use full redraw only when necessary. Cursor composited independently from scene where possible; no stale cursor trails.
- Bounded frame pacing and event coalescing without losing button/key transitions, drag or resize semantics. Preserve serial recovery and PS/2.
- Test real C renderer with synthetic surfaces, pitch/padding canaries, edge and overlap cases, high-frequency input, deterministic frames, allocation failures; run existing regressions, build ISO, verify extraction. Test actual guest mouse movement and window operations with QEMU/OVMF or VirtualBox if available. Compare framebuffer writes/redraw count, not just LOC.
- Double buffering avoids exposing the blank intermediate frame; tearing may remain without scanout synchronization. Do not claim flicker fixed on hardware until tested there.

### Other open issues recorded at Build 35
- Physical xHCI USB activation, real IOMMU translation, interrupt input delivery and physical validation absent; PS/2 is prior confirmed input path.
- Ring 3 GUI actual IRETQ/yield/timer behavior guest validation pending; single fixed client only. Ten desktop tools still kernel-trusted.
- Volatile RAMFS and policy table; no general persistent storage. Live disk write remains disabled.
- Build 35 full aggregate regression not complete; several previous builds also had time limits. Re-establish clean full test baseline.
- ISO size need not grow on every checkpoint; verify kernel/EFI hashes and image report. Retain all historical logs (Build 31 packaging regression).
- See existing MISSING.md for detailed open scope. No new hardware or security guarantee is implied by this file.
