# STATION NOVA 1.4 — native webMAN autoplay test

**Separate PS3 app, STN000014.** This builds alongside the user-approved 1.2 (STN000012) and 1.3 (STN000013), with no overwrite of earlier versions.

## Proven baseline (unchanged)

- Identical Tiny3D cinematic UI, game covers, layout, hero, navigation, custom XMB icon and progressive cover loading as versions 1.2/1.3.
- Identical confirmed mounting command on PS3 and encrypted PS2: `snprintf(command,sizeof(command),"/mount_ps3%s",encoded)` followed by `webman_http(command)`. No changes to game scanning or mounts.
- User confirmed both platforms mount, but v1.2 and v1.3 each returned to XMB instead of automatically starting games.
- R2 + X still requests **mount-only** and returns the user to XMB, preserving the previously working two-step path.

## Why 1.4 changes approach

Inspection of the upstream webMAN MOD source showed:

1. `/play.ps3/<game path>` invokes the native **force autoplay** route, not merely a play-disc request. Code: `include/cmd/pad_combo_play.h` rewrites the request to mount and sets `ap_param=2`.
2. When handling this mode, `include/www/www_client.h` invokes `auto_play(param, 1)`.
3. `auto_play` in `include/mount/mount.h` waits up to 15 seconds for the in-game state to end, then attempts to launch via the XMB. This path also contains logic for encrypted BIN.ENC via PS2 Classics Launcher.

Thus, v1.4 retains the known-good `/mount_ps3` request **first**, but replaces v1.3's unsupported/unverified temporary batch-launch mechanism with an asynchronous `/play.ps3<encoded full path>` request. This may cause webMAN to invoke its own mount logic a second time, but NOVA does not change its original mounting behaviour. NOVA exits in about eight render frames to release control promptly.

WebMAN's PS2 Classics auto-launch branch depends on the relevant webMAN PS2 Launcher setting (`ps2l`) and installed Placeholder. NOVA does not alter settings or assume a hard-coded EBOOT path.

**No claim of console verification.** The plugin must accept the second request, wait for XMB, and launch the game on the user's real HEN system. It may still return to XMB without launching. If it does, the already-mounted XMB icon remains the reliable fallback.

## Safety

- No direct retail EBOOT spawn.
- No persistent `/dev_hdd0/onxmb.bat` hook, launch marker or plugin installation.
- No 1.3 `.bat` launch script: a stale `/dev_hdd0/tmp/nova_13_play.bat` is removed on NOVA 1.4 startup, without touching other files.
- Game ISOs, PS2 BIN.ENC files, saves, webMAN configuration and HEN settings are not modified.
- Art-safe mode: hold L2 during opening to bypass PNG decoding.
- If HEN freezes or the console unexpectedly restarts, stop testing. The user-tested 1.2 is still installed.

## Controller

| Action | Controller |
| --- | --- |
| Mount and request native webMAN autoplay | X |
| Mount only and exit to XMB | R2 + X |
| Select game | D-pad |
| Jump PS3 / PS2 shelf | L1 / R1 |
| Search | Square or Select |
| Refresh | Triangle |
| Exit / close message | Circle |

## Source references

- webMAN official web commands: https://github.com/aldostools/webMAN-MOD/wiki/Web-Commands
- webMAN source: https://github.com/aldostools/webMAN-MOD/blob/master/include/cmd/pad_combo_play.h
- webMAN autoplay logic: https://github.com/aldostools/webMAN-MOD/blob/master/include/mount/mount.h
- Proven visual build: https://github.com/Oxacobe/PS3-View/tree/recovery/nova-1.2-visuals-confirmed-perfect
- Previous 1.3 build: https://github.com/Oxacobe/PS3-View/tree/recovery/nova-1.3-before-native-autoplay