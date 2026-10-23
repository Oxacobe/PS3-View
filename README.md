# STATION NOVA 1.6 — one-press launch, root cause fixed

**Separate installer, STN000016.** Installs alongside 1.2–1.5; overwrites nothing.

## Why 1.2–1.5 mounted but never launched (from webMAN-MOD source)

1. `include/www/www_client.h`: a `/mount_ps3` received **while a game is running** (NOVA counts as a game) is rewritten to `/mount.ps3`. That branch mounts and **never schedules autoplay**.
2. `include/cmd/pad_combo_play.h`: a **bare** `/play.ps3` (used by 1.2 and 1.5) calls `exec_xmb_item(col, seg)` with **empty strings**, not NULL, so the "game / disc icon" defaults are never applied. There is no target to press.
3. `include/init/vsh.h`: `wait_for_abort()` returns immediately while still in-game. Any launch step that runs before NOVA has fully closed silently gives up.
4. `include/mount/mount.h` (1.4's `/play.ps3/<path>`): waits only 15 s for the game to close, then quits if the XMB's explore_plugin hasn't loaded yet.
5. 1.5's `wm_request` file is only polled by the **Full** edition with the PhotoGUI/"Poll wm_request" option on, and still carried the bare `/play.ps3`.

## What 1.6 does on X

1. Same proven `/mount_ps3<path>` as 1.1–1.5 (unchanged, awaited).
2. One HTTP multi-command chain to webMAN (supported: `www_client.h` splits on `;/`):

```
/wait.ps3?3 ; /wait.ps3?xmb ; /wait.ps3?5 ;
/popup.ps3?NOVA 1.6: XMB ready, starting game ;
/play.ps3?col=game&seg=seg_device        (PS3 disc icon)
/play.ps3?col=game&seg=<PS2 Classics ID> (PS2, e.g. PS2U10000)
```

3. NOVA exits. webMAN waits until NOVA is gone and the XMB is loaded, then presses X on an **explicit** XMB icon — the same icon you already start manually.

## Reading the test

| What you see | Meaning |
| --- | --- |
| Popup, then the game starts | Fixed |
| Popup, game does not start | Chain survives; XMB item press is the problem. Report which platform |
| No popup at all | webMAN's chain did not run after NOVA exited |

Don't touch the D-pad, L2 or Circle on XMB during those ~10 seconds: webMAN aborts autoplay on those buttons by design.

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
