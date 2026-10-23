# STATION · PlayStation 3 library

A standalone PS3 homebrew library browser for PS3HEN and webMAN MOD. This project is **independent of** webMAN, HEN, IRISMAN, PS2 Classics Placeholder, the games and savedata.

**Status:** Early native proof of concept. The first release scans game directories and draws a controller-operated library without mounting or altering anything. Do not consider this a finished launcher yet.

## Goals
- Native PS3 Game-menu app, fullscreen cover gallery for PS2 and PS3.
- Read the existing internal game directories; no copying or renaming games.
- Use the existing webMAN mounting/PS2 Classics launcher pathways when verified.
- Cloud-compiled with GitHub Actions: no local developer tools on a Mac.

## Game locations
- PS2 Classics: `/dev_hdd0/PS2ISO/*.BIN.ENC`
- PS3 ISO: `/dev_hdd0/PS3ISO/*.iso`
- PS3 folder: `/dev_hdd0/GAMES/<title>/PS3_GAME/`

The current native prototype is strictly **read-only**, and selecting a game only displays its source path. Cover display and launching are follow-up phases, not claimed to work already.

## Builds
Open **Actions → Build STATION PS3 → Run workflow**. The build uses a third-party PS3 toolchain container and uploads compiled artifacts. Only install when the workflow passes and we've checked the output.

Native PS3 compatibility must be tested on the console. A successful GitHub compile is not the same as PS3HEN validation.
