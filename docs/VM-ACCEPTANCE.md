# Guest acceptance — Awakens Build 38

Keep the original Farkuad 0.13.1 ISO available as a fallback. Attach `build/Farkuad-Live.iso` to a disposable VirtualBox VM configured with EFI x64, 1 CPU, 512–1024 MiB RAM, VBoxSVGA, PS/2 Mouse, 3D disabled, network disabled and no virtual disk. No USB tablet. Do not change host Secure Boot or write physical media until this VM check succeeds.

| Test | Expected outcome |
|---|---|
| UEFI menu | ASCII ogre, exact slogan, AWAKENS DEV 38 R1, ten-second timeout |
| Default Live boot | Windowed Terminal and taskbar |
| Keyboard | Type `version`, Enter; reports 0.20.0-dev.38-r1 |
| History | Execute `help`, `version`; Up/Down recalls commands; typing a draft then Up/Down restores it |
| Mouse | Drag and resize Notes; minimize, restore, maximize and close |
| Cursor damage | Move PS/2 quickly over Notes and the taskbar. Check for full-screen flashes and cursor trails, including after drag and resize. Record VirtualBox display adapter, resolution and scaling. |
| Notes | Type text, click Save; Terminal `ramcat notes.txt` shows it; modify Notes, click Load to restore |
| RAMFS | `ramwrite x.txt test`, `ramcp x.txt y.txt`, `rammv y.txt z.txt`, `ramcat z.txt`, `ramrm z.txt` |
| Read-only storage | `ls /bin`; `gpt` rejects physical access |
| Native app | `run-console /bin/sdk-demo` prints SDK marker |
| Reboot | RAMFS files vanish; second menu option preserves classic keyboard-only UI |

Record the actual outcome of each check. Capture a screenshot or serial log for failures. The checks above are instructions, not results from this environment.

- Open the Swamp menu, choose the second-column tools, check RAM/HEX content, change palette, paint and save/load. Confirm the classic option still opens.

- Run `rammkdir docs`, `ramwrite docs/hello.txt hello`, `ramls docs`, `ramcat docs/hello.txt`, `ramrmdir docs` (must reject), `ramrm docs/hello.txt`, `ramrmdir docs`.

- Run `run /bin/gui-demo` (exit 13), then `gui-demo` (GUI_DEMO_OK). Open Paint and press Shift+G: the native demo should appear in the canvas.
