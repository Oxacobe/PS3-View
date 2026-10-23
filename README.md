# STATION NOVA — stable recovery build

**Current safe version: NOVA 0.5.** The user confirmed that NOVA 0.5 opens and launches PS3 games directly on a real console.

**Do not install NOVA 0.6:** the expanded four-row/32-cover experiment caused a black screen and froze the console. That experimental code is preserved separately at [quarantine/nova-0.6-freeze](https://github.com/Oxacobe/PS3-View/tree/quarantine/nova-0.6-freeze) for debugging, but has been rolled back on main.

## Recover a frozen PS3

Use the physical PS3 power button to shut down if the controller cannot respond. Restart normally and allow any system file check to complete. Don't format storage or delete games to fix this application problem.

## Installation

1. Open [GitHub Actions builds](https://github.com/Oxacobe/PS3-View/actions/workflows/build.yml).
2. Choose a successful **NOVA 0.5 / rollback** build, or use the original successful build #31.
3. Download `station-ps3-build` and extract `station.pkg`.
4. Enable HEN, insert the USB drive, and install through the PS3 Package Manager. NOVA uses a different title ID from the original STATION and is installed alongside it.

## Capabilities

- PS3 direct game launching through webMAN mounting followed by `sysProcessExitSpawn2`: user-tested working in NOVA 0.5.
- PS2 Classics launching: not fully validated. Launcher detection and the experimental redesign from 0.6 are quarantined because of the freeze.
- Scans `/dev_hdd0/PS3ISO`, `/dev_hdd0/GAMES` and encrypted PS2 Classics in `/dev_hdd0/PS2ISO`.
- Controller navigation and local search.

## Development policy

Treat a successful GitHub Actions build as a **compile/package check only**, not a console runtime test. Future visual overhauls should be staged on branches and keep a release candidate free of large startup cover loads until real hardware acceptance is complete.

The previously tested code is preserved at [recovery/ps3-one-press-confirmed-v0.5](https://github.com/Oxacobe/PS3-View/tree/recovery/ps3-one-press-confirmed-v0.5).
