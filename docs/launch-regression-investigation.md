# STATION NOVA launch-regression investigation — 23 October 2026

## Verified on physical PS3 (user reports, not cloud build)

| Version | PS3 mounting | PS3 one-press launching | PS2 mounting | PS2 one-press launching | Other |
|---|---|---|---|---|---|
| 0.5 | Yes | **YES, user-confirmed** | Unverified | **Not verified; hardcoded PS2U10000 detection failed** | Direct process-spawn path worked for a real PS3 game |
| 0.7 | Not disputed | **NO** (same game launched through webMAN outside NOVA) | Yes | No confirmed success | Four-row UI opened |
| 0.9 | Yes | **NO, black screen and exit/reboot** | Yes | No | Direct spawn into mounted EBOOT could disrupt system |
| 1.1 | **YES** | Manual XMB disc start works | **YES** | Manual XMB PS2 Classics Placeholder works | Reliable two-stage mounting baseline |
| 1.2 | **YES** | **NO** (returned to XMB) | **YES** | **NO** | Visuals user-approved exactly; keep frozen |
| 1.3 | **YES** | **NO** (same as 1.2) | **YES** | **NO** | Temporary script solution did not complete |
| 1.4 | **YES** | **NO** (same behaviour) | **YES** | **NO** | Native webMAN force-autoplay request did not complete |

Do not rewrite this matrix as if both consoles were previously **proven** to auto-launch in one press. The user has previously reported a PS2 Classics handoff in earlier versions, but the recoverable conversation/source confirms mounting and use of the installed Placeholder on XMB, not a successful one-press PS2 launch.

## Core failures in source

1. Version 0.5 calls `sysProcessExitSpawn2("/dev_bdvd/PS3_GAME/USRDIR/EBOOT.BIN",...)` about 850ms after `/mount_ps3`. The user directly confirmed PS3 launching. In later development we abandoned this working PS3 path because a subsequent similar direct-spawn implementation caused a black screen and the system rebooted. The exact cause of that difference is **not known**. Simply pasting back the syscall is not a validated repair.
2. Versions 1.2–1.4 share the network helper `webman_queue_xmb_launch()`: it writes the GET request and immediately calls `shutdown` and `close`, without reading the HTTP response or receiving any confirmation that webMAN parsed, retained, or executed the request. The function returning `1` only means the request bytes were handed to the socket. Prior claims that this proved a webMAN command was queued/accepted were incorrect.
3. Version 1.2 relied on the concatenated URL `/wait.ps3?xmb;/play.ps3`. The webMAN batch engine supports semicolon-delimited commands in scripts; its HTTP `/wait.ps3` endpoint does not clearly promise interpreting a semicolon suffix as a second HTTP action. The approach was not verified.
4. Version 1.3 created a temporary `.bat` file and requested `/play.ps3/<file>.bat`, but did not establish whether the request was handled, whether the script was executed, whether `wait xmb` returned, or whether `/play.ps3` acted. The system simply returned to XMB.
5. Version 1.4 requested webMAN native `/play.ps3/<game-path>` after the successful `/mount_ps3`, which may invoke a second mount. The plugin implementation uses an auto-play function that may wait up to 15s for the app to exit, but if compiled without the requisite launch support or configured differently this may not start. We have no response/trace proving it even reached that code path.
6. webMAN MOD PS2 encrypted `.BIN.ENC` autoplay can depend on the PS2 launcher setting `ps2l`, XMB grouping (`XMB_GROUPS`) and the prepared Classics ISO path. Manual PS2 Classics launch working does **not** establish the auto-play branch will work. See `include/mount/mount.h`. Do not guess that the installed Placeholder is missing because one hardcoded path fails.

## Frozen, reliable assets

- The **1.2** Tiny3D four-row cinematic UI, its icon, source and incremental cover handling were explicitly user-approved. Branch: `recovery/nova-1.2-visuals-confirmed-perfect`.
- The **1.1** mount-only flow was confirmed for both platforms. Branch: `recovery/nova-cinema-1.1-confirmed-mount`.
- The **0.5** direct PS3 spawn source was user-confirmed. Branch: `recovery/ps3-one-press-confirmed-v0.5`.
- The 0.6 artwork-memory freeze remains quarantined. Do not restore synchronous decoding of 32 PNGs.

## Required diagnostic sequence BEFORE claiming next launch fixed

1. Stop shipping successive PKGs with an arbitrary new HTTP command. Establish **whether** webMAN receives and accepts the second-stage launch request in the XMB context. The existing helper does not verify this.
2. Test one mounted PS3 game using webMAN `/play.ps3` **while already at XMB**, and independently test the actual installed PS2 Classics Placeholder. These are external/on-console tests, not claims inferred from C source.
3. If the XMB-context PS3 command works, the remaining issue is timing/queue/delivery after NOVA closes, not the mount or disc itself. Use an instrumented, recoverable plugin-side handoff design and verify actual execution of that command. Do not overwrite pre-existing `/dev_hdd0/onxmb.bat` or change HEN configuration without explicit agreement.
4. For PS2, verify the actual Launcher ID and webMAN `ps2l` setting; do not assume default `PS2U10000` simply because the command is documented for a standard installation.
5. If considering restoring 0.5's syscall, compare the **complete** launch path against the later black-screen versions and test in a separate app with a fail-safe exit; do not claim it is universally safe merely because it once worked.
6. New acceptance test: same selected game is confirmed launched on physical hardware from **a single X press in NOVA** for PS3 and PS2 separately, with a safe mount-only fallback and no freezes/reboots across repeated starts.

### Bottom line

The visual design and webMAN mounting have been solved. The asynchronous launch handoff remains **unverified**, not 'almost working'. Do not conflate GitHub Actions build green with console runtime behaviour. Explicitly state what is known, unknown and what the next controlled test establishes.

Relevant upstream code: https://github.com/aldostools/webMAN-MOD/blob/master/include/cmd/pad_combo_play.h, https://github.com/aldostools/webMAN-MOD/blob/master/include/mount/mount.h, https://github.com/aldostools/webMAN-MOD/blob/master/include/feat/script.h

## Update — root causes confirmed in webMAN-MOD source (NOVA 1.6)

- `/mount_ps3` while IS_INGAME is rewritten to `/mount.ps3`, which never sets `ap_param`: mount-only by design (`www_client.h`, `mount_copy.h`).
- Correction to item 3 above: HTTP multi-commands ARE supported; `www_client.h` splits on `;/`. 1.2's chain failed for other reasons: bare `/play.ps3` passes empty `col`/`seg` strings to `exec_xmb_item`, so no XMB item is targeted; and if explore_plugin stayed loaded during NOVA, `/wait.ps3?xmb` returned at once and the launch ran while still in-game, where `wait_for_abort()` returns immediately.
- 1.5: `wm_request` is polled only with PHOTO_GUI (Full edition) and the "Poll wm_request" option, and it still sent bare `/play.ps3`.
- 0.5 and 0.7 used identical direct-spawn code yet behaved differently; direct EBOOT spawn bypasses XMB boot context and is game-dependent. Not used in 1.6.

1.6 sends `/wait.ps3?3;/wait.ps3?xmb;/wait.ps3?5;/popup.ps3?...;/play.ps3?col=game&seg=<seg_device|PS2 title ID>`. The popup separates "chain never ran" from "XMB press failed". Still requires console confirmation.
