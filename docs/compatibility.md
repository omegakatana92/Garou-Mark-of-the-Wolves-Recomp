# Hardware compatibility report

Game: **Garou: Mark of the Wolves** (MAME set `garou`, NGM-2530, 1999)
Board model: **MVS**, BIOS `sp-s2.sp1` (Asia/Europe v2)

This report records what was implemented and what was actually observed. A
"verified" line means a run of the deliverable executable produced the stated
behaviour; it is not a claim about every path in the game.

## 68000 CPU

| Item | State |
|---|---|
| Instruction set | All 68000 instructions emitted by `m68krecomp` |
| Registers / flags | Native helpers reproduce Musashi's results, including undocumented flags |
| Exceptions / interrupts | Real 68000 frames; IRQ1 VBlank, IRQ2 raster timer, IRQ3 power-on |
| Reset | BIOS reset vector, watchdog reset, and bus reset |
| Verification | `--verify` 9,658,446 blocks, 0 mismatches (8,000-frame gameplay run) |
| Verification | `--verify` 4,822,016 blocks, 0 mismatches (4,200-frame coin/start run) |
| Conformance | Upstream toolkit corpus 59/60 (same as the reference interpreter); not rerun as part of this port |

## Program ROM and protection

| Item | State |
|---|---|
| Original set | `253-ep1..ep4` 2 MB each, NEO-SMA encrypted; `kf.neo-sma` 256 KB |
| Decryption | MAME `garou_decrypt_68k()` ported to `tools/decrypt.c`; NEO-GEO header and vector sanity checked |
| Layout | 9 MB image: 1 MB fixed + 8 MB banked; `kf.neo-sma` at `$0C0000..$0FFFFF` |
| Banking | NEO-SMA 6-bit selector at `$2FFFC0`, 64-entry 64 KB-granular table, bit order `{12,14,6,7,9,5}` |
| Protection read | `$2FE446` returns `$9A37` |
| RNG | 16-bit LFSR at `$2FFFCC` and `$2FFFF0`, seed `$2345` |
| Watchdog | Reset after 1,622,015 68k clocks without a write to `$300001` |
| Verified | BIOS boot, coin/start, menus, match |

## Graphics

| Item | State |
|---|---|
| C ROMs | 8 x 8 MB CMC42-encrypted, paired odd/even into 64 MB |
| Decryption | MAME `cmc42_gfx_decrypt()` with key `0x06`, `sfix_decrypt()` |
| S ROM | 512 KB extracted from the end of the decrypted C data |
| Sprites | Per-scanline renderer, sticky chains, L0 vertical shrink, horizontal shrink, 96/line, auto-animation |
| Fixed layer | 512 KB banked with Garou's 32-line VRAM table (`$7500`/`$7580`); `NG_FIX_BANK_GAROU` |
| Palettes | Both palette banks, 5-bit guns and dark bit |
| Raster | LSPC timer IRQ (IRQ2), VBlank at line 240 |
| Verified | SNK eye-catcher, intro art, title, HOW TO PLAY, T.O.P. SYSTEM, player select portraits, stage, sprites, HUD, cutscene |

## Audio

| Item | State |
|---|---|
| M1 | 256 KB plain Z80 program, not encrypted on this board |
| Z80 | Real driver, NMI command latch, reply latch, ROM banking |
| YM2610 | FM, SSG, ADPCM-A/B through ymfm, clocked by the Z80 |
| Output | SDL2 audio or `--wav` |
| Verified | 3,000-frame WAV, 2 ch / 55,555 Hz, peak 22238, RMS 2152.5 |
| Not verified | Subjective mix quality (not auditioned) |

## I/O and input

| Item | State |
|---|---|
| Controllers | Keyboard and up to two SDL2 GameControllers |
| Coin / start / service / test | Verified through the BIOS |
| RTC | uPD4990A serial calendar model |
| System latch | Vector swap, fix/Z80 ROM select, backup RAM lock, palette bank |
| Memory card | Not implemented (toolkit limitation) |
| Physical gamepad | Not available in this environment; mapping is the toolkit's tested SDL path |

## Host platform

| Item | State |
|---|---|
| Windows x64 | MSVC 19.51, Release, SDL2 2.32.10; headless and windowed |
| Linux x86_64 | gcc 11.4, SDL2 2.0.20; headless |
| AppImage | Built under Ubuntu 22.04, glibc 2.35; tested under WSL2 |
| Steam Deck hardware | Not tested |
| Fullscreen | 4:3 pillarboxed 1280x800 on a 16:10 display; F11 toggles |

## Not implemented / untested

* Memory cards.
* AES system ROMs and PAL timing.
* Physical Steam Deck (LCD/OLED), Steam Input and Gaming Mode.
* Physical gamepads.
* Subjective audio quality.
* Native execution of code in swapped bank windows (deliberate interpreter
  fallback; measured in the README).
