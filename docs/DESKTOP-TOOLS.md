# Desktop tools — Build 04

The second launcher column opens the new utilities. Tab cycles through all ten tools; Esc returns to Terminal. The taskbar shows either the first or second group of five windows, depending on focus. To restore a minimized window in another group, use the launcher.

| Tool | Controls | Behavior |
|---|---|---|
| RAM Files | J/K | Select a RAMFS file; the selected file is shared with Hex |
| Hex | J/K file, N/P offset | Read actual RAMFS bytes in hexadecimal |
| Paint | WASD (lowercase), Space, 0–5, C (lowercase), Shift+S/L, click on canvas | Move cursor, draw, choose ink, clear, save/load `paint.swpaint` |
| Settings | 1–3 | Change desktop background palette in RAM |
| Guard | J/K | Scroll snapshot of existing kernel audit ring; no permission mutation |

Paint files contain exactly 288 indexed-color bytes (24×12, values 0–5). An invalid file is rejected without replacing the canvas. Save overwrites `paint.swpaint` in the same volatile store as Notes. RAM Files is a browser, not a full GUI file manager; use the shell for file operations. Hex does not expose kernel memory. Audit is a read-only snapshot refreshed on desktop redraw, not a live security monitor. All new views run in the trusted kernel desktop; isolation of graphical clients is still an explicit milestone. No physical disk writes or network activation are introduced.

Build 06: Shift+G in Paint loads the native demo frame after successful completion. This is separate from loading `paint.swpaint` and does not persist data.
