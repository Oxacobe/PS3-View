# STATION NOVA 1.9 — PS3 one-press confirmed; PS2 gets its own XMB segment

**Separate installer, STN000019.** Installs alongside earlier versions; overwrites nothing.

## Why 1.2–1.5 mounted but never launched (from webMAN-MOD source)

1. `include/www/www_client.h`: a `/mount_ps3` received **while a game is running** (NOVA counts as a game) is rewritten to `/mount.ps3`. That branch mounts and **never schedules autoplay**.
2. `include/cmd/pad_combo_play.h`: a **bare** `/play.ps3` (used by 1.2 and 1.5) calls `exec_xmb_item(col, seg)` with **empty strings**, not NULL, so the "game / disc icon" defaults are never applied. There is no target to press.
3. `include/init/vsh.h`: `wait_for_abort()` returns immediately while still in-game. Any launch step that runs before NOVA has fully closed silently gives up.
4. `include/mount/mount.h` (1.4's `/play.ps3/<path>`): waits only 15 s for the game to close, then quits if the XMB's explore_plugin hasn't loaded yet.
5. 1.5's `wm_request` file is only polled by the **Full** edition with the PhotoGUI/"Poll wm_request" option on, and still carried the bare `/play.ps3`.

## Console results (user-confirmed)

- **1.6 PS3: the game started from a single X press.** First one-press PS3 launch since 0.5. That flow (in-game `/mount_ps3`, then a chained `/play.ps3?col=game&seg=seg_device` run by webMAN after NOVA exits) is kept unchanged in 1.8.
- 1.6 PS2: the chain pressed X on the NOVA icon (NOVA re-opened), proving the chain survives exit and X is pressed; `seg=<title id>` was not a valid XMB segment, so focus never moved.
- 1.7 replaced the working PS3 route with `/play.ps3<path>`; it was never console-tested and is superseded.

## What 1.9 does on X

- **PS3:** identical to 1.6/1.8 (console-confirmed).
- **PS2:** the Placeholder lives in the plain installed-apps list (photo: between Apollo and mmCM), which no XMB segment name can target, and HEN has no `/app_home` segment for webMAN's `/play.ps3?<titleid>` route (that is why 1.8 pressed X on NOVA). So NOVA gives the Placeholder a segment of its own: HEN's `category_game.xml` has hook entries (`hen_toolbox_xmb`, `henbrew_store_xmb`, `ps3xploit_xmb`, `game_xmbm_hdd0`, `external_xmb`) that load an XML file if present. NOVA reads the console's own `category_game.xml`, picks the first unused hook, and writes a one-item XML whose query selects the Placeholder by title ID (the same `xcb://0/query?limit=1&cond=Ae+Game:Game.titleId PS2U10000` webMAN uses for its own PS2 launcher entry). With no icon/title table the result is spliced inline, like `seg_device`. The chain then runs `/xmb.ps3$home*` (reload Game column) and `/play.ps3?col=game&seg=<hook>` — the same focus-segment-then-push route that launches PS3 discs in 1.6.

Side effect: a second "PS2 Classics Placeholder" icon appears near the top of the Game column. The XML stays so the icon persists across reboots; delete `/dev_hdd0/game/HENTOOLBX/USRDIR/hen_toolbox_xmb.xml` (or whichever hook the launch log names) to remove it.

Chain (PS2): `/wait.ps3?3 ; /wait.ps3?xmb ; /wait.ps3?5 ; popup step 1 ; /xmb.ps3$home* ; /wait.ps3?3 ; /play.ps3?col=game&seg=<hook> ; popup step 2`

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
