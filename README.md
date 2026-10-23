# STATION 0.3 — PS3 + PS2 Classics library

STATION is a native PlayStation 3 homebrew game-library interface that works alongside webMAN MOD, HEN and PS2 Classics Launcher.

## Release status

STATION 0.3 has **compiled and packaged successfully on GitHub Actions**. The original 0.1 was tested on PS3 hardware and 0.2.1 started, but the new 0.3 interface and game-mount handoff still require physical-console verification. A successful cross-compile alone does not establish console functionality.

## What's new

- New high-contrast charcoal/black theme, white text, red focus accent.
- Redesigned PS3 XMB application artwork.
- Two visible cover rails: PlayStation 3 above PlayStation 2, five cards per rail.
- Full-height artwork preserving original proportions, titles beneath covers, horizontal scrolling.
- Search inside your collection with a controller-driven keyboard. SPACE, DELETE, DONE, CLEAR are built in.
- Optional ONLINE keyboard action asks webMAN to open a public MobyGames search in the PS3 browser; website compatibility on an older PS3 browser is not guaranteed.
- webMAN mount request uses its documented `/mount_ps3/<path>` in-game bypass. STATION confirms webMAN's HTTP response and exits to XMB for you to select the mounted game icon.

## Controller

| Button | Action |
| --- | --- |
| Up/Down | Switch between PS3 and PS2 rails |
| Left/Right | Browse horizontally through games |
| X | Mount highlighted game using webMAN |
| Square / Select | Open local-search keyboard |
| Triangle | Rescan library |
| Circle | Close message / exit STATION |

On the search keyboard, navigate with D-pad and press X to enter characters or select SPACE, DELETE, DONE, ONLINE and CLEAR.

## Mounting and launching

- **PS3 ISO and folder games**: Request `/mount_ps3/dev_hdd0/...` from webMAN on 127.0.0.1, return to XMB, then select the mounted disc icon.
- **PS2 Classics `.BIN.ENC`**: Requires the installed `PS2U10000` PS2 Classics Launcher. Mount through webMAN, return to XMB and select the PS2 Classics Launcher.
- We intentionally no longer promise seamless one-button automatic startup from inside running STATION: the earlier `/play.ps3` attempt dropped out to XMB without starting the selected game. The PS3/browser/plugin environment makes auto-start from running homebrew less reliable than mounting then launching from XMB.
- STATION's library scanner does not copy, modify or delete games or saved data. webMAN may perform its normal mount operations when you press X.

## Scanned paths

- PS3 ISO: `/dev_hdd0/PS3ISO/*.iso`
- PS3 folder games: `/dev_hdd0/GAMES/*/PS3_GAME/`
- PS2 Classics: `/dev_hdd0/PS2ISO/*.BIN.ENC`

Cover lookup: filename.PNG, basename.PNG, lowercase variations, PS3_GAME/ICON0.PNG and some webMAN cache names. Unfound covers show a visible fallback rather than leaving the whole tile blank.

## Installation

1. Open [STATION builds](https://github.com/Oxacobe/PS3-View/actions/workflows/build.yml).
2. Select the most recent **green** run.
3. Download the `station-ps3-build` artifact and extract `station.pkg`.
4. Copy it to a FAT32 USB drive, enable HEN and install in **Package Manager > Install Package Files > Standard**.
5. Open the refreshed STATION icon in the Game menu.
6. First check the interface and both shelves. Then mount a known-good PS3 ISO and confirm its disc icon exists on XMB. Lastly test a known-good encrypted PS2 Classic.

## Recovery

The former 0.2.1 code is preserved on the [recovery/station-0.2.1 branch](https://github.com/Oxacobe/PS3-View/tree/recovery/station-0.2.1). The original v0.1 installed successfully on the console and can be recovered from successful GitHub build #6.

## Build details

The cloud build runs PSL1GHT / Tiny3D. XMB art and a DejaVu font atlas are compiled into the package using Pillow on GitHub Actions, with no developer tools required on your Mac. The external container dependency is not yet pinned by image digest.
