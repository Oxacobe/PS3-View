# STATION NOVA 1.2 — Cinematic PS3 & PS2 Library

**Title ID: STN000012.** This is a separate install from confirmed-mounting NOVA CINEMA 1.1 (STN000011). No PS3 dev tools are required on the user's Mac; cloud GitHub Actions builds the native PS3 PKG.

## What's already known

On the user's console, 1.1 mounts **both** PS3 ISOs/folder games and encrypted PS2 .BIN.ENC games through webMAN MOD. Launching the mounted PS3 disc icon or manually opening the PS2 Classics Placeholder/Launcher on XMB works.

1.2 preserves the identical proven webMAN mounting request (/mount_ps3 with full encoded game path). It adds **experimental one-press launching**, and a substantial actual console-native UI redesign. The one-press handoff is NOT YET CONSOLE VERIFIED.

## One-press launch, no fragile direct EBOOT spawning

Press **X** on a real library cover. NOVA mounts the game via webMAN as in 1.1, then sends a **separate asynchronous** HTTP command to the webMAN plugin:

- PS3: /wait.ps3?xmb;/play.ps3
- PS2 Classics: /wait.ps3?xmb;/play.ps3?PS2U10000

webMAN officially documents multiple commands separated by semicolons and its wait-for-XMB, mounted-disc play and title-ID play commands. For installations with a nonstandard PS2 Classics Launcher title ID, NOVA also scans installed PARAM.SFO entries whose title is **PS2 Classics Launcher** or **PS2 Classics Placeholder**, without trying to run arbitrary executables.

The HTTP request is sent then the socket is closed without waiting for a response, allowing NOVA to exit back to XMB. webMAN should complete the queued launch when VSH has returned. **This is a hypothesis to test on-console; HTTP send success is not proof the game has started.** If it doesn't auto-start, the game should remain mounted for manual starting. Stop testing if the console freezes or restarts.

Hold **R2+X** to skip auto-play and mount only. This preserves the console-confirmed 1.1 method even if auto-play is incompatible with the particular webMAN/HEN version.

No direct retail EBOOT spawn, no permanent onxmb.bat script, no marker writes, no HEN, game, save or webMAN-config changes. At startup, NOVA clears only the two obsolete marker files created by NOVA 1.0 (nova_xmb_ps3.once / nova_xmb_ps2.once), not unrelated system data.

## Actual PS3-native graphics, no browser and no mockup

The Tiny3D source has a **new 848x512 cinematic interface** based on the user's supplied reference: a slim navigation rail with glowing orange N emblem, a star-filled hero with a copper planet and horizon silhouette, dynamic title display, orange glowing selection border, clean sidebar sections, portrait cover thumbnails and a compact controller legend. Exactly **eight covers per row in two PS3 rows and two PS2 rows**. These are real scanned game covers, never imaginary mockup titles.

The 1.1 progressive PNG cover loading, 3MB gallery-texture cap, protective PNG validation and L2 art-safe mode are retained. Hero effects render with bounded Tiny3D quads instead of expensive new cover textures. XMB icon is generated separately as a graphite/orange NOVA brand identity.

## Controls

| Button | Action |
|---|---|
| X | Mount and request automatic launch after returning to XMB |
| Hold R2 + X | Mount only; manually launch disc or PS2 Classics icon on XMB |
| D-pad | Move among covers |
| L1 | Focus PS3 shelf |
| R1 | Focus PS2 shelf |
| Square/Select | Search game titles |
| Triangle | Rescan collection |
| Circle | Close/exit |
| Hold L2 during start | Disable decoding of artwork for the session |

## Recovery, diagnostics, and limits

- Console-tested **mounting baseline**: branch recovery/nova-cinema-1.1-confirmed-mount.
- Earlier **32-cover freeze**: 0.6 is quarantined. Do not install 0.6.
- Failed direct EBOOT attempts could blank/reboot the PS3; those calls are not in 1.2.
- Successful GitHub Actions compilation and PKG packaging do NOT prove the queued XMB handoff will work on hardware.
- Last mount/handoff stage recorded at /dev_hdd0/tmp/station_nova_launch.log (overwrites previous log).
- If a launch fails but the selected disc or Placeholder can be opened manually, report that observation. If HEN becomes unstable, stop rather than repeating the attempt.

References:
- https://github.com/aldostools/webMAN-MOD/wiki/Web-Commands
- https://github.com/aldostools/webMAN-MOD/wiki/Game-Paths-%26-Covers

Build status: https://github.com/Oxacobe/PS3-View/actions/workflows/build.yml
