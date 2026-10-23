# STATION NOVA 1.5 — XMB-deferred one-press launcher candidate

**A separate native PS3 installer, STN000015.** Installs alongside the 1.2 visual/mount baseline, 1.3 and 1.4; does not overwrite them.

## What has already worked on the user's actual PS3

- Version 0.5: PS3 direct launch worked at least once; later similar direct-spawn attempts crashed, so that mechanism is NOT restored.
- Version 1.1 and later: webMAN `/mount_ps3` successfully mounts PS3 games and encrypted PS2 BIN.ENC games.
- Version 1.2's premium Tiny3D cinematic interface, generated icon and streaming 32-cover library have been explicitly approved.
- Version 1.2–1.4: attempted asynchronous HTTP/script automatic handoffs FAILED. They returned to XMB without starting a game.

## Why 1.5 is fundamentally different

After the unchanged, successful `/mount_ps3` request, NOVA 1.5 does NOT send another HTTP request or create an XMB event script.

Instead, it exclusively creates `/dev_hdd0/tmp/wm_request`, webMAN MOD's **own deferred-command mailbox**.

By inspecting upstream webMAN MOD, we established:

1. `include/poll/poll.h` polls that request file only when not in-game, so it can be consumed after NOVA exits and XMB returns.
2. `include/www/www_client.h` reads the file, adds `GET` to an endpoint that begins with `/`, retries `/play.ps3` while still in-game (up to 30 seconds), and deletes the file before executing.
3. `include/cmd/pad_combo_play.h` defines `/play.ps3` as the mounted PS3 disc launch command, and `/play.ps3?TITLEID` as the launch of an installed title such as the PS2 Classics Launcher.

NOVA writes **only the endpoint**, never a complete HTTP request, because webMAN's parser rewrites spaces in the queued text.

### Single X press

- PS3: mount selected PS3 ISO/folder game through the unchanged `/mount_ps3<encoded full path>`; then create `wm_request` containing exactly `/play.ps3`; exit normally.
- PS2: mount the selected encrypted BIN.ENC through the exact same known-good method; use the detected installed PS2 Classics Launcher title ID, normally PS2U10000, to write `/play.ps3?TITLEID`; exit normally.
- webMAN, not NOVA, detects its mailbox from the XMB context and attempts the final disc-icon / Classics Launcher action.
- If the global mailbox already exists, NOVA refuses the *automatic* attempt rather than overwriting another application's pending webMAN command. No game is mounted in that case; the previous mount-only mode remains available.

**Status: designed and cloud-compiled, NOT YET TESTED ON THE USER'S CONSOLE.** It depends on this webMAN plugin build enabling `WM_REQUEST` and polling. HTTP source-code analysis alone cannot guarantee a specific installed SPRX supports it.

## Safety and fallback

- **R2+X** is the console-tested **mount only** behaviour from 1.2. It bypasses queue creation.
- X is the automatic request-file handoff; if the game doesn't launch, it should remain mounted for manual starting from XMB.
- No new plugin, permanent startup script, EBOOT spawn, firmware change, game modification or saved-data operation.
- NOVA never overwrites/deletes a foreign `/dev_hdd0/tmp/wm_request`. If that file remains after an unsuccessful test, **do not repeatedly retry**; use IrisMAN to inspect the file first. A pending command could run on a later return to XMB.
- The only other cleanup is the same old NOVA-owned `.bat` from v1.3, if present, and two obsolete NOVA-specific v1.0 markers already cleared in earlier builds.
- If HEN freezes/restarts, stop testing immediately and use version 1.2. Its source and packaging remain preserved separately.

## Full graphical experience

**Zero intentional Tiny3D visual changes** from the user-approved 1.2/1.3/1.4 artwork: starfield and copper planet header, clean dark sidebar, eight portrait covers across in two PS3 + two PS2 rows, dynamic selected-title display, orange selection glow, controller legend, low-stress cover streaming, 3 MB gallery budget and L2 art-safe startup mode.

## Controls

| Input | Function |
| --- | --- |
| X | Mount selected game, enqueue XMB launch through webMAN's `wm_request` mailbox |
| R2 + X | Mount only, open mounted PS3 disc / PS2 Classics on XMB manually |
| D-pad | Navigate game covers and four rows |
| L1 / R1 | Focus PS3 / PS2 library |
| Square / Select | Search |
| Triangle | Rescan |
| Circle | Exit |
| Hold L2 when opening | Disable artwork decoding |

## Evidence and references

- Investigative record: `docs/launch-regression-investigation.md`.
- User-approved, frozen 1.2 source: `recovery/nova-1.2-visuals-confirmed-perfect`.
- Previous native force-autoplay attempt: `recovery/nova-1.4-before-request-file-handoff`.
- webMAN poll: https://github.com/aldostools/webMAN-MOD/blob/master/include/poll/poll.h
- webMAN request-file interpreter: https://github.com/aldostools/webMAN-MOD/blob/master/include/www/www_client.h
- webMAN game launch: https://github.com/aldostools/webMAN-MOD/blob/master/include/cmd/pad_combo_play.h
- Cloud build: https://github.com/Oxacobe/PS3-View/actions/workflows/build.yml