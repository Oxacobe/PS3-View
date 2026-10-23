# STATION NOVA ALL 0.9 — PS3 + PS2 one-press launcher test

**This is a separate native PS3 app** with title ID `STN000009`, XMB name **STATION NOVA ALL**. It does not overwrite STATION NOVA 0.7 or NOVA PLAY 0.8. It keeps the four-row / eight-column streaming cinematic gallery, cover-safe L2 mode and original PS3 direct-spawn handoff.

## Restored PS2 Classics integration

We located the exact original PS2 Classics integration from STATION 0.2 commit `6e5eb6a`. It mounted the selected encrypted `.BIN.ENC` game through webMAN MOD, then called **`/play.ps3?PS2U10000`** to request the standard PS2 Classics Placeholder/Launcher.

Later versions accidentally removed that code, replacing it with a hard-coded launcher-path check that said “PS2 Classics Launcher not found” even though Classics had been installed.

**NOVA ALL 0.9** tries the more direct Classics handoff when the installed launcher EBOOT and prepared `ISO.BIN.ENC` are visible, but **automatically restores the original v0.2 webMAN title-ID request as a fallback** if either of those checks fails or the process-spawn call returns. Both routes begin from one **X** button press in the NOVA UI.

*Limit:* an HTTP success only verifies acceptance by webMAN; it does not guarantee that HEN and the XMB actually start the PS2 game. This is a targeted on-console test. No previous STATION source version has been unambiguously recorded as launching PS2 games end to end every time.

## PS3 compatibility

The user confirmed that NOVA 0.5 launched PS3 games directly, while NOVA 0.7 opened its new four-row gallery but failed to launch one test game even though the same game launched through webMAN. We have preserved the `sysProcessExitSpawn2` process handoff from 0.5 and the mount-readiness checks from 0.8. Do not change your HEN plugin configuration solely for this app.

## Visuals & controls

- Eight covers per row, **two PS3 rows and two PS2 rows** simultaneously
- Streaming PNG thumbnails; basic validation and 3MB GPU texture budget
- **Hold L2 while opening** to disable artwork decoding if a particular cover freezes the PS3
- D-pad move across or between rows, **X launch**, Square/Select search, Triangle refresh, Circle exit

## Diagnostic log

After any launch attempt, the latest stage is written to `/dev_hdd0/tmp/station_nova_launch.log` for inspection with IrisMAN. A stage of `ps2_legacy_play_ps3` means the original webMAN Classics route was requested; `ps2_classics_spawn` means direct Classics EBOOT execution was attempted. A record is not proof that the game started.

## Install / recover

Download a successful GitHub Actions artifact at [Build STATION PS3](https://github.com/Oxacobe/PS3-View/actions/workflows/build.yml), extract `station.pkg` and install through the HEN Package Manager. This app installs separately as `STN000009`; leave 0.7 in place while testing.

A previous 0.6 build caused an actual console freeze; do not reinstall 0.6. The preserved source points are:

- [NOVA 0.7 gallery recovery](https://github.com/Oxacobe/PS3-View/tree/recovery/nova-0.7-ui-and-streaming)
- [NOVA 0.5 PS3-confirmed handoff](https://github.com/Oxacobe/PS3-View/tree/recovery/ps3-one-press-confirmed-v0.5)

Cloud compilation is **not** a physical-console launch or safety test. Only test on HEN when enabling HEN normally is stable. Don't repeatedly retry an action if the console freezes.
