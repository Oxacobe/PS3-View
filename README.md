# STATION NOVA PLAY 0.8 — launcher isolation build

This is a **separate test app** (PS3 title ID `STN000008`; XMB name `STATION NOVA PLAY`). Installing it does **not** replace the user's running STATION NOVA 0.7 (`STN000004`).

## What is retained

- **Exactly the 0.7 gallery implementation:** eight covers across each of four visible rows (two PS3 rows, two PS2 rows), dark Netflix-inspired styling, progressive cover loading and L2 cover-safe mode.
- **The original v0.5 confirmed PS3 handoff:** calls `sysProcessExitSpawn2` on `/dev_bdvd/PS3_GAME/USRDIR/EBOOT.BIN` with the same parameters. The code for this syscall was already identical in 0.7. The user confirmed 0.5 launched PS3 games and separately confirmed webMAN can launch the same PS3 title when 0.7 failed.
- All existing game-folder scanning and search.

## Launch-specific changes in 0.8

1. Mount selected game using webMAN MOD HTTP command `/mount_ps3<path>` as before.
2. Wait up to five seconds for the mounted target executable to actually exist. Earlier builds assumed 850ms was sufficient.
3. **PS3:** direct handoff to the mounted `/dev_bdvd/PS3_GAME/USRDIR/EBOOT.BIN`.
4. **PS2:** search the standard PS2 Classics Launcher/Placeholder, and possible alternate installed launcher titles; mount the encrypted `.BIN.ENC`, require the prepared `/dev_hdd0/game/PS2U10000/USRDIR/ISO.BIN.ENC` to exist, then attempt the launcher process handoff.
5. When it can't proceed, give a specific error on screen and preserve the mounted title for manual XMB launch.
6. Write a tiny per-attempt diagnostic log to `/dev_hdd0/tmp/station_nova_launch.log` (overwrites the previous log). No game or save data is changed.

webMAN MOD documentation: https://github.com/aldostools/webMAN-MOD/wiki/Web-Commands and https://github.com/aldostools/webMAN-MOD/wiki/Game-Paths-%26-Covers

## Known limits

- No source version has been user-confirmed to launch **both PS3 and PS2** directly. The 0.5 PS3 launch was confirmed; 0.5's PS2 branch rejected the placeholder as missing. PS2 remains unverified.
- A successful cloud build proves the PKG compiles, **not** that either launch path works with every PS3/HEN combination.
- Do not enable HEN with R2 held when testing webMAN launching: R2 can temporarily skip startup plugins. Use normal HEN activation only if it is now stable, or fix the plugin issue before testing.
- Version 0.6 caused a console freeze. Its source is quarantined; version 0.7 UI is frozen in the branch [recovery/nova-0.7-ui-and-streaming](https://github.com/Oxacobe/PS3-View/tree/recovery/nova-0.7-ui-and-streaming). The earlier PS3-confirmed launcher is preserved in [recovery/ps3-one-press-confirmed-v0.5](https://github.com/Oxacobe/PS3-View/tree/recovery/ps3-one-press-confirmed-v0.5).

## Installing and testing

1. Enable HEN *normally*, but only if HEN activation is stable.
2. Install `station.pkg` for `STATION NOVA PLAY` with Package Manager. Leave 0.7 installed.
3. Open the NEW STATION NOVA PLAY title in XMB, not the old NOVA.
4. Test one PS3 game known to work through webMAN, then test one encrypted PS2 title with a functioning PS2 Classics Placeholder.
5. If launching fails, note the exact message or inspect `/dev_hdd0/tmp/station_nova_launch.log` via IrisMAN. The final stage narrows down mount versus target-path versus process-handoff problems.
6. If the console becomes unresponsive, don't repeat the test: physically power off normally, restart and use the previous NOVA 0.7 app.
