# STATION NOVA 0.7 — Streaming Cinema (experimental)

NOVA is a native PS3 cover-browser and game launcher using HEN, webMAN MOD and the PS2 Classics Launcher where available. It is separate from the original STATION installation (PS3 title ID: `STN000004`).

## Why 0.7 exists

The 0.6 experiment displayed 32 cover locations but **froze a real PS3 at startup**. Its code synchronously decoded every cover before drawing the first frame, which may have caused severe startup stalls or memory stress. This has **not been conclusively established** as the cause. Do not install 0.6.

0.7 restores the requested **eight columns × four rows** (two PS3 and two PS2 shelves), while changing the cover loader fundamentally:

- Draw the UI first; wait eight frames before any PNG decoding.
- Load just one image per frame rather than all 32 synchronously.
- Reject oversized or invalid PNG headers before decoding.
- Scale each texture to at most 160 px on its longest edge.
- Limit gallery textures to 3 MB, rather than filling the whole 20 MB reserved arena.
- Render a readable no-art placeholder for missing or rejected covers.
- **Emergency art-safe mode:** hold **L2** when starting NOVA to disable all artwork loading for that session. The collection and launcher remain usable.
- Keep PS3 games on the top two rows, PS2 on the bottom two; D-pad moves through the grid and horizontal paging displays more titles.

## Launching

**PS3 direct launching was confirmed by the user on a real PS3 with NOVA 0.5.** We preserved the same mount-through-webMAN then `sysProcessExitSpawn2` handoff, and archived the v0.5 source at [recovery/ps3-one-press-confirmed-v0.5](https://github.com/Oxacobe/PS3-View/tree/recovery/ps3-one-press-confirmed-v0.5).

**PS2 Classics launching is not confirmed.** NOVA searches the normal PS2U10000 EBOOT path and installed title metadata for compatible alternative PS2 Classics launchers. If none can be confirmed, it still attempts to mount the encrypted `.BIN.ENC` via webMAN and tells you to open your installed PS2 Classics Launcher from XMB instead of wrongly blocking mounting.

## Controller

- **D-pad**: navigate the 4-row / 8-column grid
- **X**: mount and attempt direct launch
- **Square / Select**: local search keyboard
- **Triangle**: rescan library
- **Circle**: back/exit
- **Hold L2 on startup**: turn off all cover decoding (safe mode)

## Paths

- PS3 ISO: `/dev_hdd0/PS3ISO`
- PS3 folder games: `/dev_hdd0/GAMES`
- PS2 Classics encrypted games: `/dev_hdd0/PS2ISO/*.BIN.ENC`
- Covers alongside games and some webMAN cache filenames

## Cloud build / installation

The Github Actions workflow compiles with PSL1GHT + Tiny3D and packages the installable `station.pkg`, no Mac developer tools. Download from [Build STATION PS3](https://github.com/Oxacobe/PS3-View/actions/workflows/build.yml), copy to FAT32 USB and install with HEN enabled using Package Manager → Install Package Files → Standard.

**Important:** A successful GitHub build proves code compiles and packages; it does not prove 0.7 is safe on your hardware. After the 0.6 freeze, first launch with **L2 held**, verify the four-row UI and controller navigation, then reopen normally to test progressive artwork. If there is another black screen, use the physical power button to shut down and reinstall the [original console-tested NOVA 0.5 package from build #31](https://github.com/Oxacobe/PS3-View/actions/runs/39938914649).

The failed 0.6 implementation is preserved on [quarantine/nova-0.6-freeze](https://github.com/Oxacobe/PS3-View/tree/quarantine/nova-0.6-freeze) for debugging.
