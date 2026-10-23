# STATION · PS3 View

An independent, native PS3 game-library interface for PS3HEN, webMAN MOD and PS2 Classics.

**STATION 0.2 — beta, hardware test required for launch paths.** Version 0.1 was confirmed to start successfully on a physical PS3. Version 0.2 adds modern rendering, more cover fallbacks and a webMAN handoff. A green GitHub build proves the installer compiled, **not** that game launching has been verified on the console.

## Interface

- Contemporary dark gallery with antialiased DejaVu Sans text, clean navigation rail and clear selected-game state
- PS2/PS3 tabs and eight cover tiles per page
- Cover images shown at their original aspect ratio, not stretched
- Checks local PNG files alongside games and common webMAN cover/cache conventions
- No games changed by the STATION library-scanning logic

### Game sources

- PS2: `/dev_hdd0/PS2ISO/*.BIN.ENC`
- PS3 ISO: `/dev_hdd0/PS3ISO/*.iso`
- PS3 game folders: `/dev_hdd0/GAMES/<name>/PS3_GAME/`

## Controls

| Button | Function |
| --- | --- |
| L1 / R1 | Switch PS2 / PS3 |
| D-pad | Navigate games |
| X | Ask webMAN to mount/play selected game |
| Square | Toggle details |
| Triangle | Scan games again |
| Circle | Close details, or exit STATION |

## Launch integration — please read before testing

STATION talks to **webMAN MOD on 127.0.0.1:80** using its published HTTP routes:

- **PS3 ISO/folder:** `/play.ps3/<absolute game path>`. This asks webMAN to mount the game and start it from the XMB, then STATION exits.
- **PS2 Classics (.BIN.ENC):** verify PS2 Classics Launcher `PS2U10000` is present, call `/mount.ps3/<absolute game path>`, then call `/play.ps3?PS2U10000` to request the installed PS2 Classics Launcher. This is an **experimental handoff** and must be checked on your console.

If PS2 launch does not complete, the encrypted game may already have been mounted by webMAN; return to XMB and launch PS2 Classics Launcher manually. The app never directly copies, deletes or renames encrypted ISO files. webMAN may manage those files as part of its own normal PS2 mount operation.

**Requirements:** HEN enabled, compatible webMAN MOD running, PS2 Classics Launcher for PS2 games, compatible and properly encrypted PS2 games. STATION cannot guarantee that all PS3 games or PS2 Classics run on every console.

## Install without a Mac toolchain

1. Go to [GitHub Actions builds](https://github.com/Oxacobe/PS3-View/actions/workflows/build.yml).
2. Select the newest **green** build.
3. Download artifact `station-ps3-build` and unzip it.
4. Copy **`station.pkg`** to the root of a FAT32 USB stick.
5. On PS3 enable HEN, then **Game > Package Manager > Install Package Files > Standard**.
6. Open **STATION** in the PS3 Game menu.

Do not delete the original working 0.1 artifact until version 0.2 has been tested.

## Cloud build

The GitHub Actions workflow builds inside `ghcr.io/02900/ps3-toolchain-tiny3d:latest`. It also installs Ubuntu's `python3-pil` and `fonts-dejavu-core` packages, then generates an antialiased bitmap atlas embedded in the binary at build time. No runtime font or cloud connection is needed just to draw the interface.

Build dependencies are not yet pinned to immutable container digests.

## Safety boundaries

- STATION is a separate homebrew app with ID `STN000001`; it does not replace webMAN, HEN or PS2 Classics Launcher.
- The library scanner and UI do not move or alter games, configuration files or saved games.
- On X, STATION asks existing webMAN MOD to perform mounting/launching. **Those requests do have console-side effects managed by webMAN**.
- This release has not been hardware-verified for launching yet. Test with a known-working PS3 title first, then a known-working PS2 Classics title.
