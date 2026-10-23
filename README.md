# STATION NOVA 1.3 — XMB auto-launch test

**Separate PS3 app, title ID STN000013.** Installs alongside the user-tested 1.2 (STN000012). Build #55 produced a real installable PKG.

## Guaranteed source-preservation

The full user-approved 1.2 native Tiny3D visuals, cover handling, game scanning and original webMAN mounting call are unchanged. The new PKG's ICON0.PNG is byte-for-byte identical to 1.2.

**Known working behaviour on the user's actual console:** Press X to mount a PS3 ISO/folder game or encrypted PS2 BIN.ENC using webMAN. Version 1.2 mounts both. The user could then manually launch via the PS3 disc icon or PS2 Classics Placeholder on XMB.

## Changed in 1.3: only post-mount XMB launching

After the unchanged successful /mount_ps3 HTTP call, NOVA creates a small temporary script at /dev_hdd0/tmp/nova_13_play.bat with the documented webMAN commands:

    wait xmb
    wait 3
    if xmb
      del /dev_hdd0/tmp/nova_13_play.bat
      /play.ps3
    end if

For PS2, the final command instead reads /play.ps3?<CLASSICS_ID>, normally PS2U10000. NOVA scans the installed PARAM.SFO descriptions for PS2 Classics Launcher or Placeholder to accommodate alternative title IDs; it does not directly start any EBOOT.BIN.

After writing the script, NOVA sends webMAN the async request /play.ps3/dev_hdd0/tmp/nova_13_play.bat, then exits normally. webMAN's own batch interpreter supports the script's wait-xmb, if-xmb, del and play commands. This differs from 1.2's concatenated /wait.ps3?xmb;/play.ps3 URL that did not complete the XMB launch.

The script deletes itself BEFORE calling play, avoiding a repeated launch next time. If interrupted before deletion, NOVA clears that specific temporary script at its next startup. It does not install any onxmb.bat, background plugin or HEN modifications.

**This automatic launch is experimental and not yet console-confirmed.** WebMAN may reject or not execute this asynchronous script depending on configuration/version. A build success or the HTTP request being sent does not prove the game launched.

## Safe fallback and controls

- X: mount, then request automatic launch after XMB.
- **R2+X: mount ONLY**, keeping the previous console-tested manual start through the mounted PS3 disc icon or PS2 Classics icon.
- D-pad: move among 4 rows, 8 games per row.
- L1/R1: PS3 / PS2 shelf focus.
- Square/Select: search; Triangle: rescan; Circle: exit.
- Hold L2 while opening: no cover decoding.

If any launch freezes/reboots the console, stop testing and use existing 1.2. Do not modify games, saves, webMAN configuration, HEN or boot plugins.

Diagnostic log: /dev_hdd0/tmp/station_nova_launch.log; queued_unconfirmed means only that the command was transmitted.

## Source and documentation

- Frozen visual and mounting baseline: https://github.com/Oxacobe/PS3-View/tree/recovery/nova-1.2-visuals-confirmed-perfect
- webMAN commands: https://github.com/aldostools/webMAN-MOD/wiki/Web-Commands
- WebMAN implementation: include/feat/script.h and include/cmd/pad_combo_play.h
- Cloud installer workflow: https://github.com/Oxacobe/PS3-View/actions/workflows/build.yml