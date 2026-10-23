# STATION NOVA CINEMA 1.1 — premium two-stage PS3 / PS2 library

**PS3 application:** STATION NOVA CINEMA, separate title ID `STN000011`. It does not replace any previous NOVA version. **New XMB icon**: custom graphite/orange folded-N wordmark; **new 4-row interface**: modern cinema treatment of the existing 0.7 GPU gallery.

## The known reliable workflow

The user's console has successfully launched PS3 games from **webMAN MOD**, and PS2 games by **mounting their encrypted .BIN.ENC with webMAN, then selecting PS2 Classics Launcher/Placeholder from XMB**. On HEN these two are different launch flows.

From NOVA CINEMA:

1. Open the app with HEN enabled normally and webMAN running.
2. Choose an installed PS3 ISO/folder game or encrypted PS2 `.BIN.ENC` and press **X**. NOVA calls webMAN `/mount_ps3<encoded-game-path>`. It does **not** attempt to start `EBOOT.BIN` itself.
3. NOVA shows which XMB icon to open and exits cleanly.
4. **PS3:** choose the now-mounted *disc* game icon under XMB's Game column to start the PS3 game.
5. **PS2:** choose your **PS2 Classics Launcher / Placeholder** icon under XMB's Game column to start the already-mounted game.

**This is intentionally two-stage, not one-click auto-play.** It's an explicit recovery to the console's previously successful methods, and it does not pretend a cloud build proves more than that. Once this route is verified end-to-end, we can add optional auto-launch without risking these known-good basics.

## Bugs removed since 0.9–1.0

- No direct `sysProcessExitSpawn2` of retail PS3 EBOOTs or PS2 Placeholder EBOOTs (caused black screens/restarts).
- No hard-coded check of `/dev_hdd0/game/PS2U10000/USRDIR/EBOOT.BIN` (falsely reported PS2 Placeholder missing).
- No new `onxmb.bat` scripts, no marker writes, and no automatic launch triggers.
- At app startup, NOVA removes **only** old NOVA-1.0 marker files `/dev_hdd0/tmp/nova_xmb_ps3.once` and `/dev_hdd0/tmp/nova_xmb_ps2.once`. It does **not** modify an existing `onxmb.bat`; a previous NOVA-owned copy remains inert without marker files.
- Never rewrite ISO, BIN.ENC, PS2 CONFIG, webMAN config, installed games, or save files.

If you wish to manually remove old NOVA 1.0's `/dev_hdd0/onxmb.bat`, **only rename it if its first line is exactly** `# STATION NOVA XMB HANDOFF 1.0`. Preserve an `onxmb.bat` of any other origin.

## Interface

- Preserved **eight covers per row, two PS3 rows and two PS2 Classics rows**.
- Refreshed premium graphite/glass/orange visual language. XMB icon now custom angular N with strong typography.
- Streaming one cover at a time, 3 MB gallery texture cap, safe PNG screening, aspect-ratio preservation.
- **Hold L2 while opening:** artwork-safe mode to skip cover decoding.
- **X:** webMAN mount then return to XMB. **Square/Select:** library search. **Triangle:** rescan. **Circle:** exit. D-pad navigates the 4-row grid.
- After a mount attempt, diagnostics: `/dev_hdd0/tmp/station_nova_launch.log`.

## Hardware caveat

The package is compiled in the cloud with PSL1GHT/Tiny3D via GitHub Actions. A successful build does not guarantee a particular PS3/HEN installation has no plugin instability. Do **not** test this while HEN is still freezing or returning disabled. If you are recovering from 1.0, check for pending launcher markers before using XMB.

## Sources & history

[webMAN MOD game paths and PS2 Classics requirements](https://github.com/aldostools/webMAN-MOD/wiki/Game-Paths-%26-Covers)

[webMAN MOD official mount and XMB play commands](https://github.com/aldostools/webMAN-MOD/wiki/Web-Commands)

Preserved project source milestones:

- [PS3 handoff 0.5](https://github.com/Oxacobe/PS3-View/tree/recovery/ps3-one-press-confirmed-v0.5)
- [NOVA 0.7 gallery](https://github.com/Oxacobe/PS3-View/tree/recovery/nova-0.7-ui-and-streaming)
- [XMB 1.0 experimental handoff](https://github.com/Oxacobe/PS3-View/tree/recovery/nova-1.0-xmb-experiment)
