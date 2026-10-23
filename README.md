# STATION NOVA 1.7 — webMAN-native launch from XMB

**Separate installer, STN000017.** Installs alongside earlier versions; overwrites nothing.

## Why 1.2–1.5 mounted but never launched (from webMAN-MOD source)

1. `include/www/www_client.h`: a `/mount_ps3` received **while a game is running** (NOVA counts as a game) is rewritten to `/mount.ps3`. That branch mounts and **never schedules autoplay**.
2. `include/cmd/pad_combo_play.h`: a **bare** `/play.ps3` (used by 1.2 and 1.5) calls `exec_xmb_item(col, seg)` with **empty strings**, not NULL, so the "game / disc icon" defaults are never applied. There is no target to press.
3. `include/init/vsh.h`: `wait_for_abort()` returns immediately while still in-game. Any launch step that runs before NOVA has fully closed silently gives up.
4. `include/mount/mount.h` (1.4's `/play.ps3/<path>`): waits only 15 s for the game to close, then quits if the XMB's explore_plugin hasn't loaded yet.
5. 1.5's `wm_request` file is only polled by the **Full** edition with the PhotoGUI/"Poll wm_request" option on, and still carried the bare `/play.ps3`.

## What the 1.6 console test proved

- PS2: the chain pressed X on the NOVA icon (NOVA re-opened). So the chained request survives NOVA's exit and webMAN does press X. `seg=<title id>` was simply not an XMB segment.
- PS3: the disc icon was focused but the press did not start it.

## What 1.7 does on X

- **PS3:** no in-game mount. After NOVA exits, webMAN runs its own documented `/play.ps3<path>` ("mount <path> and start game from disc icon", forced autoplay) from XMB context — the same route as clicking the game in webMAN's browser page.
- **PS2:** the proven in-game `/mount_ps3` prepares the Classics placeholder; after NOVA exits, webMAN runs `/play.ps3?<Classics title ID>` (start an installed title) from XMB context.

Chain sent in both cases:

```
/wait.ps3?3 ; /wait.ps3?xmb ; /wait.ps3?6 ;
/popup.ps3?NOVA 1.7 step 1: XMB ready ;
<launch command> ;
/popup.ps3?NOVA 1.7 step 2: launch command finished
```

## Reading the test

| What you see | Meaning |
| --- | --- |
| Step 1 popup, game starts | Fixed |
| Step 1, then step 2, no game | webMAN ran the launch and it returned without starting the game |
| Step 1 only, no game | launch command hung or was aborted (D-pad / L2 / Circle pressed?) |
| No popup | chain did not run after NOVA exited |

## Controls

| Input | Function |
| --- | --- |
| X | Mount, then webMAN starts it from XMB |
| R2 + X | Mount only (console-verified fallback) |
| D-pad | Navigate covers and rows |
| L1 / R1 | Focus PS3 / PS2 library |
| Square / Select | Search |
| Triangle | Rescan |
| Circle | Exit |
| Hold L2 when opening | Disable artwork decoding |

Interface, icon and cover streaming are unchanged from the approved 1.2. Launch log: `/dev_hdd0/tmp/station_nova_launch.log`.

## References

- https://github.com/aldostools/webMAN-MOD/blob/master/include/www/www_client.h
- https://github.com/aldostools/webMAN-MOD/blob/master/include/cmd/pad_combo_play.h
- https://github.com/aldostools/webMAN-MOD/blob/master/include/init/vsh.h
- https://github.com/aldostools/webMAN-MOD/blob/master/include/mount/mount.h
- https://github.com/aldostools/webMAN-MOD/blob/master/include/poll/poll.h
