# STATION · PS3 View

A standalone PS3 homebrew game-library interface for PS3HEN, webMAN MOD and PS2 Classics.

**Status: v0.1 native hardware test.** GitHub Actions successfully compiles this into a real PS3 `station.pkg`. It has **not** been run on the actual console yet.

## Current features
- Custom STATION icon in the PS3 **Game** menu.
- Fullscreen dark-mode game gallery with controller navigation.
- Separate PS2 and PS3 categories (L1/R1).
- Reads existing internal-drive games:
  - `/dev_hdd0/PS2ISO/*.BIN.ENC`
  - `/dev_hdd0/PS3ISO/*.iso`
  - `/dev_hdd0/GAMES/<game folder>/PS3_GAME/`
- Displays PNG covers if found beside the game. For a PS2 game `Cars (USA).BIN.ENC`, its artwork is `Cars (USA).BIN.ENC.PNG`. For PS3 folder games, it reads `PS3_GAME/ICON0.PNG`.
- D-pad navigation, X for detected file details, O to exit, Triangle to rescan.

## Safety first
This v0.1 test intentionally **does not launch or mount games**. It does not delete, copy, convert or modify games, webMAN, HEN, PS2 Classics Placeholder or saved data.

The next phase will add webMAN mounting and PS2 Classics handoff **only after this read-only build has been validated on the user's PS3**. Compiling a PKG is not proof of console compatibility.

## Cloud build and download
1. Open [the latest build workflow](https://github.com/Oxacobe/PS3-View/actions/workflows/build.yml).
2. Select a successful run.
3. Download the `station-ps3-build` artifact ZIP.
4. Extract **`station.pkg`** from the ZIP. The archive also contains binaries for diagnostics.
5. Copy `station.pkg` to a FAT32 USB drive and use the PS3's package installer with HEN enabled.

No developer tools are needed on your Mac. The build uses a third-party PS3 toolchain container from `ghcr.io/02900/ps3-toolchain-tiny3d:latest`. This dependency is not yet pinned to a reproducible digest.

## Project notes
- Homebrew application ID: `STN000001` — separate from other installed tools.
- `source/main.c`: native Tiny3D application.
- `assets/create_icon.py`: dependency-free generator for the XMB art.
- `.github/workflows/build.yml`: native cloud compiler + package archive.

STATION is under active development. Do not use it to replace any working launcher.
