# Garou: Mark of the Wolves - native static recompilation

**Garou: Mark of the Wolves (SNK, 1999 / MAME set `garou`) running as a native
PC program: its 68000 program is translated ahead of time into C and executed
against a model of the MVS board by
[neogeorecomp](https://github.com/sp00nznet/neogeorecomp).**

Two deliverables are produced:

| Platform | Artifact |
|---|---|
| Windows x64 | `dist/windows/Garou-Windows-x64/Garou.exe` |
| Steam Deck / Linux x86_64 | `dist/steamdeck/Garou_Mark_of_The_Wolves-SteamDeck-x86_64.AppImage` |

The game's own code runs natively. The real MVS system ROM boots the board,
checks its checksum and RAM, shows the SNK eye-catcher, handles coins and
credits, and calls the cartridge exactly as on hardware. There is no BIOS HLE
and no emulator frontend. The Musashi interpreter remains as the correctness
oracle and as the fallback for banked code the flat dispatch cannot safely
identify (measured below).

> **No game data here.** You need your own legally obtained dumps of the MAME
> `garou` set and the `neogeo` system ROMs. The decrypted ROMs and the
> recompiled C are generated from your dump on your machine into `build-*/`;
> they are never committed or distributed.

## Status

**v0.1.2, alpha - boots, runs attract mode and plays a full match on
recompiled code.**

| Area | State |
|---|---|
| Boot | Real MVS BIOS (`sp-s2.sp1`) -> eye-catcher -> intro -> title -> HOW TO PLAY / T.O.P. SYSTEM -> PLAYER SELECT -> stage |
| ROM selector | First run with no discoverable ROMs opens a native folder picker; choosing the encrypted set auto-decrypts to the per-user cache; `--select-rom` forces it, `--rom-path` bypasses it |
| Input / credits | Coin, Start, directions, A/B/C/D tested through the BIOS and menus |
| Gameplay | Player select (Rock), CPU opponent (Hotaru), stage, movement, attacks, win/loss sequence over a 19,000-frame scripted session |
| Graphics | Sprites, backgrounds, palettes, fix layer with the Garou 512 KB bank table, raster effects |
| Audio | **Fixed in v0.1.2**: the Z80 M1 bank mapper read every bank 64 KB too high, corrupting the music driver's data. Native and interpreter captures are continuous through character select and the early match; a MAME 0.242 reference recording has the same continuity. Subjective mix not auditioned |
| Native coverage | **100.00%** of the 19,000-frame scripted session (114 interpreted instructions, 0 uncovered); **92.38%** of a 6,000-frame no-input attract session (bank-window fallback) |
| `--verify` | 9,658,446 blocks over 8,000 gameplay frames, **0 mismatches**; 4,822,016 blocks over 4,200 coin/start frames, **0 mismatches** |
| Toolkit tests | 68000 conformance 59/60 (reference 59/60); SDL virtual-gamepad test passes on Windows (P1/P2, D-pad/stick, A/B/C/D, Start, Back-as-coin, hot-unplug) |
| Windows | Release exe built with MSVC 19.51 (VS 18 2026) x64 + SDL2 2.32.10; headless and windowed launches tested |
| Steam Deck | AppImage built and launched headless/fullscreen under WSL2 Ubuntu 22.04; Linux build reports 100.00% native over the 19,000-frame session and 0 `--verify` mismatches; **not tested on physical Deck hardware** |
| License | PolyForm Noncommercial 1.0.0 (repository license; see `LICENSE-STATUS.md`) |


### The supplied set is the original encrypted release

The `garou.zip` in `game/` is the exact MAME `garou` set (CRCs verified):

```
253-ep1..ep4.p1/p2/p3/p4   2 MB each  NEO-SMA encrypted program
kf.neo-sma                 256 KB     SMA protection data
253-c1..c8.c1..c8          8 MB each  NEO-CMC42 encrypted graphics
253-m1.m1                  256 KB     plain Z80 program
253-v1..v4.v1..v4          4 MB each  plain ADPCM samples
```

The encrypted boards have **no S ROM**: the fixed-layer data lives at the end
of the C ROMs and decrypts to 512 KB. `tools/decrypt.c` implements, at build
time:

* MAME's `sma_prot_device::garou_decrypt_68k()` - data-line swap over the
  8 MB banked image, fixed 768 KB relocation from `$710000`, and 32 KB
  address-block permutation. The 256 KB `kf.neo-sma` data is kept at
  `$0C0000..$0FFFFF`.
* MAME's `cmc42_gfx_decrypt()` with `GAROU_GFX_KEY = 0x06`, then
  `sfix_decrypt()` to extract the 512 KB fixed layer from the decrypted C
  data.
* Copy-through of the plain M1, V ROMs and the four BIOS files.

The decrypted files live only in the **build directory** (`build-*/roms`),
never in this repository or the release packages.

### Cart registers and layout the stock runtime did not model

Garou's NEO-SMA cart differs from the earlier linear-banked carts. These are
implemented generically in the vendored toolkit; `src/game.c` supplies only
data:

1. **Program bank select is at `$2FFFC0`**, with the six selector bits
   unscrambled into a 64-entry table of 64 KB-granular bank bases. The cart
   supplies the table and bit list through `ng_game_t.bank_map`.
2. **A protection read at `$2FE446` returns `$9A37`** (`prot_read_addr` /
   `prot_value`).
3. **The NEO-SMA RNG** is read at `$2FFFCC` and `$2FFFF0`; it returns a
   16-bit LFSR sequence seeded at `$2345` (MAME `sma_prot_device::random_r`).
   The generic runtime models it through `rng_read_addr` / `rng_seed`.
4. **The fixed layer is 512 KB** and uses Garou's 32-line VRAM bank table
   (`$7500`/`$7580`), not the six-column KOF2000 layout. `NG_FIX_BANK_GAROU`
   selects it in `video.c`.
5. **The MVS watchdog** resets the board after ~135 ms without a kick to
   `$300001`; late games use that as a protection check. The runtime models
   it generically.

## Requirements

* **Windows**: Visual Studio 2022/2026 with the C++ workload, CMake 3.21+,
  SDL2 2.x (the project was built against SDL2 2.32.10).
* **Linux**: gcc/g++, CMake 3.21+, make, `libsdl2-dev`.
* Your own legally obtained dumps of the MAME `garou` and `neogeo` sets.

## Quick start

### 1. Prepare your ROM files

Unzip both sets into one directory, for example `roms-src/`:

```
mkdir roms-src
unzip garou.zip -d roms-src
unzip neogeo.zip -d roms-src
```

The build runs `garou_decrypt` and writes a runtime-ready set to
`build-*/roms`. To prepare it yourself:

```
build-windows/Release/garou_decrypt.exe --rom-path roms-src --out roms
```

### 2. Build

Windows (from the project root):

```
cmake -S . -B build-windows -G "Visual Studio 18 2026" -A x64 ^
      -DGAROU_ROM_DIR=C:/path/to/roms-src ^
      -DSDL2_DIR=C:/path/to/SDL2-2.32.10/cmake
cmake --build build-windows --config Release --parallel
```

If the path to your SDL2 install is not supplied, the build is headless-only
(or fails to find the window). A headless build needs no SDL2:

```
cmake -S . -B build-headless -G "Visual Studio 18 2026" -A x64 ^
      -DGAROU_ROM_DIR=C:/path/to/roms-src ^
      -DCMAKE_DISABLE_FIND_PACKAGE_SDL2=ON
```

Linux:

```
cmake -S . -B build-linux -DCMAKE_BUILD_TYPE=Release \
      -DGAROU_ROM_DIR=$PWD/roms-src
cmake --build build-linux --parallel "$(nproc)"
```

To build the portable AppImage in one step:

```
./packaging/appimage/build-appimage.sh /path/to/roms-src
```

That writes `dist/steamdeck/Garou_Mark_of_The_Wolves-SteamDeck-x86_64.AppImage`
(SDL2 is bundled). Put your `roms/` directory next to the AppImage, or pass
`GAROU_ROM_DIR` / `--rom-path`.

### 3. Run

```
build-windows/Release/Garou.exe --rom-path build-windows/roms
# or
build-linux/Garou --rom-path build-linux/roms
```

If you start the player without `--rom-path` and no ROM folder is found in
the usual places (`roms` beside the executable, `GAROU_ROM_DIR`, the
per-user cache), it opens a native folder picker:

* **Windows:** the standard folder dialog.
* **Linux:** `zenity`, `kdialog` or `yad`; if none is available it asks on
  the terminal. In a Steam Deck Gaming Mode session without a dialog helper,
  put a `roms` folder next to the AppImage or set `GAROU_ROM_DIR` in the
  launch options.

Point the picker at either the decrypted set (the folder containing
`253-pd.bin`) or the original encrypted MAME set (the folder containing
`253-ep1.p1`). In the encrypted case the bundled `garou_decrypt` helper runs
once and writes a decrypted copy to `%LOCALAPPDATA%\Garou\roms` on Windows or
`~/.local/share/garou/roms` on Linux; later launches reuse it. Use
`--select-rom` to force the picker even when a `roms` folder exists, and
`--rom-path DIR` to bypass it entirely. `--headless` never opens a dialog.

Useful flags: `--headless`, `--frames N`, `--input FILE`,
`--screenshot N:FILE`, `--wav FILE`, `--interp`, `--verify`,
`--dump-misses FILE`, `--scale N`, `--fullscreen`, `--mute`,
`--select-rom`. Run `--help` for the runtime list.

## Controls

| Input | Action |
|---|---|
| Arrows / left stick | move |
| Z X C V / A B X Y (Xbox layout) | Neo Geo A B C D |
| 5 / Back | insert coin |
| 1 / Start | start |
| 2 | player 2 start |
| 3 / left shoulder | select |
| 9 | service credit |
| F2 | test switch |
| F11 | toggle fullscreen |
| Esc | quit |

The pad mapping is the toolkit's fixed SDL GameController layout; a pad SDL
does not recognise can be described with `SDL_GAMECONTROLLERCONFIG`.

On an MVS, insert a coin before Start does anything. The BIOS's coin/start
handling is verified.

## Testing

```
# BIOS -> eye-catcher -> intro -> title -> attract
Garou.exe --headless --rom-path build-windows/roms --frames 4000 ^
    --screenshot 600:bios.png --screenshot 1200:intro.png ^
    --screenshot 3999:attract.png

# coin -> start -> character select -> T.O.P. -> stage -> match
Garou.exe --headless --rom-path build-windows/roms --frames 19000 ^
    --input tests/play_long.txt --screenshot 7000:match.png

# replay every native block in Musashi and compare
Garou.exe --headless --verify --rom-path build-windows/roms --frames 8000 ^
    --input tests/play_long.txt

# collect runtime-only entry points, rebuild, and re-measure
cmake --build build-windows --config Release --target profile
cmake --build build-windows --config Release --target garou
```

Measured results on the Windows Release build:

```
[tests/play_long.txt, 19000 frames]
[exec] native blocks 23511399, native instructions 337317063,
       interpreted instructions 114 (100.00% native), 0 uncovered entry points

[empty input, 4000 frames]
[exec] native blocks 3464335, native instructions 73890126,
       interpreted instructions 38 (100.00% native), 0 uncovered entry points

[empty input, 6000 frames - attract demos with bank switches]
[exec] native blocks 5416682, native instructions 100880719,
       interpreted instructions 8322316 (92.38% native), 0 uncovered entry points

[--verify, tests/play_long.txt, 8000 frames]
[verify] 9658446 blocks checked, 0 mismatches
[exec] native blocks 9658446, native instructions 143014107,
       interpreted instructions 76 (100.00% native), 0 uncovered entry points
```

The recompiler translates **919,726 instructions into 42,630 entry points**
after one profile pass (before profiling: 906,495 instructions / 42,348
entries). The `profile` target runs a 6,000-frame attract session and a
19,000-frame scripted play session, merges the addresses it had to interpret
into `build/entries.txt`, and the next build regenerates the table with them.

Block counts vary by a handful between runs because the uPD4990A calendar is
seeded from the host clock; the native/interpreted instruction totals and the
coverage percentage are stable. The figures above are from the recorded runs.

With no input, the recompiled and interpreter-only builds produce
**byte-identical PNGs** at frames 600, 1200, 2500 and 3999 (checked by
SHA-256).

### Why the attract session reports 92.38%

Garou's 8 MB program is banked through the NEO-SMA mapper. The recompiler's
flat dispatch table describes the first MB of the banked image; when the game
points the window at a different 64 KB-granular bank, the same 68k address
means different bytes. Running the bank-0 routine there would be wrong, so
`exec.c` deliberately falls back to Musashi while `ng_bank_base != 0`. The
scripted 19,000-frame session stays on bank 0 after the profile pass and runs
100% natively. The attract session reaches other characters' code through
switched banks, which is the 7.62% interpreted share. This is a measured
correctness fallback, not an emulator wrapper: the fixed bank (all main code
and the BIOS) is recompiled and runs natively.

## Audio

**Root cause (confirmed):** the Z80 M1 bank mapper added a 64 KB base to the
file offset. MAME loads the M1 into an audio region with a second copy at
offset `0x10000`; the bank formula already includes that base, so the file
offset must be the masked bank value alone:

```c
/* old: wrong - reads one 64 KB block too high */
off = 0x10000 + (s_bank[w] & ((size - 0x10000 - 1) & 0x3FFFF));
/* fixed */
off = s_bank[w] & ((size - 1) & 0x3FFFF);
```

Every banked read was therefore served from the wrong part of the 256 KB M1.
The fixed bank (driver code) still ran, so commands and some effects worked,
but the music sequences and instrument tables were corrupted. The driver
eventually stalled or played isolated samples. The mask also truncated banks
above `0x2FFFF`, although Garou's traced session only used banks up to
`0x27000`; the incorrect base was the primary fault.

The correction is in `neogeorecomp/src/audio.c`:

* use the masked file offset directly (MAME `init_audio()` / `ROM_RELOAD`
  layout), and
* call `audio_sync()` before generating samples so the Z80 sequencer keeps
  running even when the 68000 is not polling a sound port.

**Evidence:**

| Run | Long silent runs after boot |
|---|---|
| Before, old audio, native 19,000 frames | 96-107 s (12 s), 205-212 s (8 s), 249-260 s (12 s) |
| Before, old audio, interpreter 19,000 frames | 96-107 s (12 s), 188-205 s (18 s) |
| After, native 19,000 frames | 105 s (1 s), 258-259 s (2 s) |
| After, interpreter 19,000 frames | no long run before 186 s; the 196-204 s quiet window is the continue screen |
| MAME 0.242 reference, same scripted inputs | 0-5 s, 16 s, 93-94 s, 105 s, 225 s, 298-302 s only |

The MAME reference has overall RMS 2850 and the repaired native capture 2813,
with the same continuity. A MAME run is not sample-identical because its
mixer and timing differ, but the long BGM dropouts are absent.

A regression test is in `tests/audio_regression.py`. It runs the scripted
session, computes one-second RMS windows and fails on any silent run longer
than 3 seconds after the boot period:

```
python tests/audio_regression.py --exe build-windows/Release/Garou.exe \
    --rom-path build-windows/roms
python tests/audio_regression.py --exe build-windows/Release/Garou.exe \
    --rom-path build-windows/roms --interp
```

Both pass on the repaired build. The old interpreter build fails with the
12-second 96-107 s gap, which is the regression this test guards.

The Z80 driver still uses the real M1 program, NMI/reply latches and ROM
banking. The YM2610's FM, SSG and ADPCM-A/B are synthesized by
[ymfm](https://github.com/aaronsgiles/ymfm), clocked by the Z80, and played
through SDL2 or written with `--wav`. The mix has not been listened to
critically; subjective quality remains unverified.

## Known limitations

* **Banked code in attract mode is interpreted.** See the coverage section.
* **Steam Deck hardware not tested.** The AppImage was built and launched
  under WSL2 Ubuntu 22.04 (headless and fullscreen); physical Deck LCD/OLED,
  Steam Input and Gaming Mode still need validation.
* **Physical gamepads not tested here.** The SDL2 GameController path is the
  toolkit's existing mapping; no physical pad was available in this
  environment.
* **Audio continuity fixed, mix not auditioned.** Native and interpreter
  captures are continuous and the regression test passes; music/effect
  balance and subjective quality were not listened to.
* **No memory card support** (the toolkit does not implement it).
* **No commercial use.** The project is licensed under PolyForm Noncommercial
  1.0.0; do not assume commercial redistribution is permitted.
* **No ROMs or BIOS included.** You must supply your own legally obtained
  dumps.

## Project layout

```
CMakeLists.txt              build: decrypt -> recompile -> player
src/game.c, game.h          verified ng_game_t: ROM layout, bank map, SMA ports
src/main.c                  player (ng_main)
src/rom_select.c/.h         first-run folder picker and encrypted-set decrypt
src/recomp_main.c           generator (ng_recomp_main)
tools/decrypt.c             NEO-SMA/NEO-CMC42 decryption, S-ROM extraction
tools/prot_cmc.cpp          MAME CMC tables (BSD-3, reference)
tools/prot_sma.cpp          MAME SMA permutations (BSD-3, reference)
tools/tables.h              generated CMC42 tables
tests/coin_start.txt        credit/start script
tests/play_long.txt         coin -> start -> character -> match script
tests/audio_regression.py   WAV continuity regression test
packaging/appimage/         AppImage build script, desktop file, icon, README
docs/                       compatibility, testing, hardware and runtime notes
neogeorecomp/               toolkit copy (MIT), with generic SMA/RNG/fix fixes
```

## Documentation

* `docs/compatibility.md` - hardware and platform compatibility report.
* `docs/testing.md` - commands and measured results.
* `docs/game-notes.md` - ROM set, decryption, banking and boot sequence.
* `docs/runtime-changes.md` - generic changes in the vendored toolkit.

## License

Original code in `src/`, `tools/decrypt.c`, `packaging/` and the documentation
is licensed under **PolyForm Noncommercial 1.0.0** (see `LICENSE` and
`LICENSE-STATUS.md`).

`neogeorecomp/` remains MIT (sp00nznet); Musashi and superzazu/z80 are MIT;
ymfm is BSD-3-Clause; the decryption tables and permutations are BSD-3-Clause
from MAME; SDL2 is zlib. Full attribution is in `THIRD_PARTY_NOTICES.md` and
the preserved `neogeorecomp/LICENSE`, `NOTICE` and `LICENSES/`.

No SNK ROMs, BIOS images, decrypted ROMs, generated recompiled C or
ROM-derived entry lists are distributed. Garou: Mark of the Wolves and Neo Geo
are trademarks of SNK Corporation. This project is not affiliated with or
endorsed by SNK. Supply your own legally obtained dump.
