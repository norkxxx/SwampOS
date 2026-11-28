# SwampGuard graphical policy editor — Build 13

Open the Swamp menu and choose GUARD. The previous audit view remains the default. Press P or click its first row to enter the editor. Six bundled ELF applications can be selected with J/K (or the right/left half of the selection row).

| Control | Action |
|---|---|
| 1–5 / permission row | Toggle the draft ceiling for console, file resource, IPC, scratch or GUI |
| 0 / right half of Apply row | Stage a deny-all ceiling |
| A / left half of Apply row | Explicitly commit changed ceiling |
| R / reload row | Reload current policy and discard draft |
| P / first row | Return to audit, discard draft |
| Close | Discard editor state |

Maximize the window to see all controls. The interface only changes launch capability ceilings. It never grants capabilities itself. The application still needs an explicit launch grant. No userspace policy-administration syscall is introduced. The editor uses the same volume-UUID + inode policy table and audited update path as the trusted shell. Revision tokens are invalidated by a successful policy update, mount attempt or unmount, so stale drafts cannot silently overwrite a different policy. A rejected edit requires reloading. The global token conservatively invalidates edits to unrelated apps too. The GUI only covers the six bundled ELF paths; the shell remains available for other validated ELF files and removal of stored profiles. The 32-slot volume-scoped table is volatile; no persistent policy storage, live revocation or multi-user authorization is claimed.

Host verification links the real editor adapter, renderer, window input, storage command parser, EXT4 reader and policy table. Native CPU entry remains mocked. A C-rendered preview is `assets/awakens-guard.png`; it is not a guest screenshot.
