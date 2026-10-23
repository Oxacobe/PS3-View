# STATION NOVA XMB 1.0 — isolated PS3/PS2 launch handoff prototype

**Separate application:** XMB title `STATION NOVA XMB`, ID `STN000010`. Installing it does **not** replace STATION NOVA 0.7, PLAY 0.8 or ALL 0.9.

## Why the launch model changed

NOVA ALL 0.9 showed a black screen for PS3 games and restarting the application after the failed launch rebooted the PS3. PS2 Classics also failed. **Do not keep testing 0.9.**

webMAN MOD documents:
- `/mount_ps3<game path>` for mounting a game from inside another game/homebrew
- `/play.ps3` for launching the mounted PS3 disc from XMB
- `/play.ps3?PS2U10000` for requesting the standard PS2 Classics Placeholder/Launcher
- `/dev_hdd0/onxmb.bat` as an event script run when XMB loads or returns.

The previous experiments called `sysProcessExitSpawn2` on disc `EBOOT.BIN` or arbitrary PS2 launcher EBOOTs. **NOVA XMB does not directly spawn commercial-game executables.** It requests execution from the XMB via webMAN.

## How one-press launching is attempted

1. User selects a PS3 or PS2 game and presses X in the same 4-row, 8-column cover gallery.
2. For PS2, require the actual `/dev_hdd0/game/PS2U10000/USRDIR/EBOOT.BIN` Placeholder. Do NOT scan or launch arbitrary installed games.
3. Ensure a tiny NOVA-owned webMAN script exists at `/dev_hdd0/onxmb.bat`, but **never overwrite or append to a user-existing different script**. If another script exists, display an error and leave the script and console untouched.
4. Mount the selected game using webMAN `/mount_ps3`.
5. Create only one of two one-use files: `/dev_hdd0/tmp/nova_xmb_ps3.once` or `/dev_hdd0/tmp/nova_xmb_ps2.once`.
6. Exit NOVA normally. On its next XMB event, webMAN's small script deletes the marker **before** launching `/play.ps3` (PS3) or `/play.ps3?PS2U10000` (PS2). This should prevent repeated auto-launch cycles.
7. No permanent resident STATION plugin is installed. The inert event script remains until removed manually, but does nothing when neither marker exists.

**This is an experimental integration.** GitHub compilation does not establish that your specific webMAN/HEN version runs the XMB event before or after returning from NOVA or launches the selected game.

## Recovery and file-system safety

- Installing the app alone does **not** create the script or markers. They are created only on a selected game's first launch attempt.
- We never modify your `boot_plugins.txt`, webMAN settings, game ISO, encryption keys, installed Placeholder or save files.
- If the console unexpectedly restarts before the one-use marker was consumed, it may be triggered the next time XMB loads. **To cancel a pending launch**, use IrisMAN to delete both `/dev_hdd0/tmp/nova_xmb_ps3.once` and `/dev_hdd0/tmp/nova_xmb_ps2.once` before re-enabling HEN. If HEN cannot enable normally, use the established plugin-bypass process first.
- If you remove this test app, you may also delete `/dev_hdd0/onxmb.bat` **only after confirming it begins with** `# STATION NOVA XMB HANDOFF 1.0`. Never delete an existing non-NOVA script.
- A diagnostic log recording the last stage is written to `/dev_hdd0/tmp/station_nova_launch.log`.

## Preserved interface

Same NOVA 0.7 layout: two PS3 rows and two PS2 rows, eight covers across, progressive cover loading, L2 no-art startup mode, controller navigation and local search.

## Recommended testing precautions

First confirm HEN enables normally and a PS3 title runs through webMAN's own XMB menu. If console behaviour remains unstable, **do not install or test this prototype**. The software is still experimental.

The exact former source versions remain available:
- [0.5 PS3 launch reference](https://github.com/Oxacobe/PS3-View/tree/recovery/ps3-one-press-confirmed-v0.5)
- [0.7 UI reference](https://github.com/Oxacobe/PS3-View/tree/recovery/nova-0.7-ui-and-streaming)
- [0.9 pre-XMB source](https://github.com/Oxacobe/PS3-View/tree/recovery/nova-0.9-before-xmb-handoff)

Documentation: https://github.com/aldostools/webMAN-MOD/wiki/Web-Commands
