# Testing log

All commands below were run against the release artifacts in this tree with
the supplied `garou.zip` and `neogeo.zip`. ROMs and BIOS are not distributed.

## 1. ROM validation

`tools/decrypt.c` checks the expected file sizes (4 x 2 MB P, 256 KB SMA,
8 x 8 MB C, 256 KB M1, 4 x 4 MB V) and the decrypted program's `NEO-GEO`
header at `$100`. The decrypted image reports `SP=0010F300 PC=00C00402`.

The MAME `garou` set CRCs of the supplied files match the standard set:

```
253-c1 0603e046  253-c2 0917d2a4  253-c3 6737c92d  253-c4 5ba92ec6
253-c5 3eab5557  253-c6 308d098b  253-c7 c0e995ae  253-c8 21a11303
253-ep1 ea3171a4 253-ep2 382f704b 253-ep3 e395bfdd 253-ep4 da92c08e
253-m1 36a806be  kf.neo-sma 98bc93dc
253-v1 263e388c  253-v2 2c6bc7be  253-v3 0425b27d  253-v4 a54be8a9
```

## 2. Interpreted boot

```
Garou.exe --headless --interp --rom-path build-windows/roms --frames 4000 \
  --screenshot 600:bios.png --screenshot 1200:intro.png \
  --screenshot 2500:title.png --screenshot 3999:attract.png
```

Observed: SNK eye-catcher, intro art, `INSERT COIN(S)`, attract match.
Screenshots at 600/1200/2500/3999 were produced.

## 3. Native compilation and pixel comparison

The same 4,000-frame run with the recompiled table produced PNGs with the
same SHA-256 hashes as the interpreter run:

```
frame 600    E4816F13...C87306FA7567B9
frame 1200   F083EFD5...F34CCE3988B0AAD
frame 2500   7D50707E...E4260E8C1CA16B6
frame 3999   E11191CC...21085C4C7E28D8FDE
```

Coverage after one profile pass:

```
[exec] native blocks 3464335, native instructions 73890126,
       interpreted instructions 38 (100.00% native), 0 uncovered entry points
```

## 4. Coin / start and menus

```
Garou.exe --headless --rom-path build-windows/roms --frames 4200 \
  --input tests/coin_start.txt --screenshot 4199:select.png
```

Observed: HOW TO PLAY, T.O.P. SYSTEM, PLAYER SELECT (Rock), 5-count timer.
At 5,000 frames the game reaches T.O.P. SET and then the stage.

## 5. Long scripted gameplay

```
Garou.exe --headless --rom-path build-windows/roms --frames 19000 \
  --input tests/play_long.txt --screenshot 7000:match.png
```

Observed: Rock vs Hotaru, stage, movement/attacks, HUD, win/loss cutscene.
Measured:

```
[exec] native blocks 23511399, native instructions 337317063,
       interpreted instructions 114 (100.00% native), 0 uncovered entry points
```

## 6. CPU verification

```
Garou.exe --headless --verify --rom-path build-windows/roms --frames 4200 \
  --input tests/coin_start.txt
[verify] 4822016 blocks checked, 0 mismatches

Garou.exe --headless --verify --rom-path build-windows/roms --frames 8000 \
  --input tests/play_long.txt
[verify] 9658446 blocks checked, 0 mismatches
```

## 7. Coverage after profiling

The `profile` target ran a 6,000-frame attract session and the 19,000-frame
scripted session, writing 282 runtime-only entry points. Rebuilding raised the
table from 42,348 to 42,630 entries and reduced the scripted session from 339k
interpreted instructions (99.56%) to 114 (100.00%).

The 6,000-frame attract session still shows 8,322,316 interpreted instructions
(92.38%) because it reaches code through swapped bank windows. This is the
documented correctness fallback, not missing entry points (`0 uncovered entry
points`).

Block counts vary by a handful between runs because the uPD4990A calendar is
seeded from the host clock. The instruction totals and coverage percentage
are stable; the first `--verify` run checked 9,658,446 blocks and a later
release-artifact run checked 9,658,433, both with 0 mismatches.

## 8. Audio

```
Garou.exe --headless --rom-path build-windows/roms --frames 3000 \
  --input tests/coin_start.txt --wav garou_3000.wav
```

WAV: 2 channels, 55,555 Hz, 2,815,503 frames. Peak 22238, RMS 2152.5, with
4,466,108 nonzero samples out of 5,631,006. Signal is present; the mix was not
auditioned.

## 9. Windowed launch

`Garou.exe --rom-path ... --frames 300 --scale 2` opened an SDL2 window and
exited after 300 frames:

```
[neogeorecomp] garou: 42630 recompiled entry points
[exec] native blocks 80425, native instructions 7231996,
       interpreted instructions 0 (100.00% native), 0 uncovered entry points
```

## 10. Linux / AppImage

Built with gcc 11.4 on Ubuntu 22.04 under WSL2. Results:

```
[Linux, tests/play_long.txt, 19000 frames]
[exec] native blocks 23511251, native instructions 337317063,
       interpreted instructions 114 (100.00% native), 0 uncovered entry points

[Linux, --verify, tests/play_long.txt, 8000 frames]
[verify] 9658559 blocks checked, 0 mismatches
[exec] native blocks 9658559, native instructions 143014107,
       interpreted instructions 76 (100.00% native), 0 uncovered entry points
```

Frames 7000 and 12000 are byte-identical to the Windows build (same SHA-256).

AppImage checks:

* The file is an executable ELF x86_64 AppImage.
* `--appimage-extract` shows `usr/bin/Garou`, `usr/bin/garou_decrypt`,
  `usr/lib/libSDL2-2.0.so.0`, the desktop file and the icon.
* `ldd` resolves `libSDL2-2.0.so.0` from the bundled `usr/lib`; the remaining
  libraries are the standard SteamOS/glibc + X11/Wayland/audio set.
* First launch with `GAROU_ROM_DIR` pointing at the encrypted set decrypts
  into `~/.local/share/garou/roms` and runs.
* Second launch with the cached set runs normally.
* `--headless` 3000-frame audio: 2 ch / 55,555 Hz, peak 22238, RMS 2152.5.
* Fullscreen launch under WSLg completed 300 frames and exited cleanly.
* Physical Steam Deck hardware was not available.

## 11. Toolkit regression tests

The vendored toolkit was built as its own project and its tests run:

Windows (SDL2 2.32.10):

```
ng_gamepad_test.exe
gamepad: virtual pad is player 1
gamepad: virtual pad is player 2
gamepad: all checks pass

ng_conformance.exe --corpus .../ext/musashi/test/mc68000 --min-pass 59
conformance: 59/60 68000 test programs pass recompiled
             (reference interpreter: 59/60)
```

Linux (SDL2 2.0.20):

```
ctest --test-dir build-conformance
1/2 Test #1: conformance ... Passed
2/2 Test #2: gamepad ..... Passed   (skipped: SDL2 2.24+ required)
100% tests passed, 0 tests failed out of 2
```

The gamepad test exercises the real `window_sdl.c` mapping with SDL's
virtual joystick: P1/P2 assignment, D-pad + stick, A/B/C/D positions, Start
and Back-as-coin, and hot-unplug. No physical pad was needed.

The other supported games in the toolkit tree use their own toolkit copies,
so these generic changes do not alter their binaries.

## 12. ROM selector

The first-run folder picker was exercised on Windows and Linux:

* Windows, empty directory, `--select-rom`: the native `IFileOpenDialog`
  folder picker opened (the process stayed in `Show()` until it was killed).
* Windows, `GAROU_ROM_DIR` pointing at the encrypted `roms-src`: the player
  ran `garou_decrypt.exe` into `%LOCALAPPDATA%\Garou\roms`, then booted and
  ran 300 windowed frames.
* Windows, `roms/` beside the executable, no `--rom-path`: the default was
  found, no dialog appeared, and 300 windowed frames ran.
* Linux, `roms/` beside the executable, no `--rom-path`: no dialog, 300
  windowed frames ran under WSLg.
* Linux, empty directory: WSL did not have zenity/kdialog/yad, so the
  terminal fallback prompted for a folder. A pseudo-TTY fed the decrypted
  path and the game ran one frame from it.
* The native Linux zenity/kdialog dialog itself was not available in the
  WSL image; it is the first branch of `pick_dir()` and is used on desktop
  Linux/SteamOS when present.

These checks cover path discovery, argument injection, encrypted-set
detection, the bundled decrypt call, the Windows native picker and the Linux
terminal fallback. A physical Steam Deck dialog was not tested.
