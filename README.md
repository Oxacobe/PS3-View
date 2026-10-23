# STATION NOVA 0.6 — Cinema Library (PS3 / PS2)

STATION NOVA is a native PlayStation 3 homebrew library alongside HEN, webMAN MOD and PS2 Classics. It has its own app ID `STN000004`, so the original STATION remains separate.

## What's new in 0.6

- Four rows permanently on screen: two for PlayStation 3 and two for PlayStation 2
- Eight games across each row, for **up to 32 visible covers**
- Black cinematic background, white font, restrained red focus, minimal borders and compact titles underneath each image
- The currently selected title is shown prominently in the header
- Horizontal D-pad browsing, vertical row switching and 16-game paging per platform
- Downsampled cover textures (maximum 256 px on longest side) to fit 32 covers into the PS3's texture memory
- Controller-operated library search; optional online game-information lookup (browser compatibility not guaranteed)
- More robust PS2 Classics Launcher discovery: the standard `PS2U10000` path plus installed packages whose PARAM.SFO identifies a PS2 launcher or placeholder

## Launching

- **PS3 one-press launch was confirmed working on a real PS3 with version 0.5.** The direct launch function is retained unchanged in 0.6. The PS3 ISO or folder game is mounted via webMAN; STATION attempts to spawn `/dev_bdvd/PS3_GAME/USRDIR/EBOOT.BIN` by `sysProcessExitSpawn2`.
- **PS2 launch is still experimental.** STATION mounts the encrypted `.BIN.ENC` via webMAN and attempts to spawn a detected PS2 Classics Launcher. If no compatible launcher is detected, STATION tells you the game was mounted and to start your installed Classics Launcher manually from XMB. This avoids incorrectly declaring the launcher missing before even trying to mount.
- STATION doesn't modify game or save files; webMAN still performs normal mounting operations.

## Controls

| Button | Action |
|---|---|
| Left / Right | Move across a row |
| Up / Down | Switch between rows and systems |
| X | Mount and attempt direct launch |
| Square / Select | Local title search |
| Triangle | Rescan game folders |
| Circle | Close message or exit |

## Scanned folders

- PS3: `/dev_hdd0/PS3ISO/*.iso` and `/dev_hdd0/GAMES/*/PS3_GAME/`
- PS2: `/dev_hdd0/PS2ISO/*.BIN.ENC`
- Cover files alongside games and common webMAN cache names

## Installation

1. Open [STATION build history](https://github.com/Oxacobe/PS3-View/actions/workflows/build.yml).
2. Select the newest green run and download the `station-ps3-build` artifact.
3. Extract `station.pkg` to a FAT32 USB stick.
4. Enable HEN on the PS3. Use **Package Manager → Install Package Files → Standard**.
5. Open **STATION NOVA** (not the original STATION) in the XMB Game menu.

## Recovery

The confirmed working PS3 direct-launch source is preserved in [recovery/ps3-one-press-confirmed-v0.5](https://github.com/Oxacobe/PS3-View/tree/recovery/ps3-one-press-confirmed-v0.5). The original v0.1 also remains available via successful GitHub Actions build #6.

**Test limitation:** GitHub Actions verifies compilation and packaging, but cannot confirm screen layout, PS2 Classics detection or launch compatibility on a real console. The 0.6 interface needs on-hardware acceptance testing.
