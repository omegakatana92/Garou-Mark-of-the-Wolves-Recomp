Garou: Mark of the Wolves - Steam Deck / Linux x86_64 AppImage (Audio Fixed)
=============================================================================

This is the native Linux x86_64 build with the Z80 M1 bank-mapping audio
repair. The game's 68000 program is statically recompiled to C and runs on
the neogeorecomp MVS board model with the real MVS BIOS. It is not an
emulator wrapper.

ROOT CAUSE
----------
The Z80 M1 bank mapper added a 64 KB base to the file offset. MAME's audio
region already places the M1 at region offset 0x10000, so the file offset is
the masked bank value alone. The old code read every banked window 64 KB too
high. The driver's fixed bank still ran, so commands and some effects worked,
but music sequences and instrument tables were corrupted; BGM stopped or
played isolated samples.

Fixed in neogeorecomp/src/audio.c:
  - use the masked file offset directly: (M1 size - 1) & 0x3FFFF
  - call audio_sync() before sample generation so the Z80 sequencer keeps
    running when the 68000 is not polling a sound port

EVIDENCE
--------
- Old native 19,000-frame capture: silent runs at 96-107 s (12 s),
  205-212 s (8 s), 249-260 s (12 s).
- Old interpreter capture: 96-107 s (12 s), 188-205 s (18 s).
- Fixed native capture: only 105 s (1 s) and 258-259 s (2 s).
- Fixed interpreter: no long run before 186 s; the 196-204 s quiet window is
  the game-over/continue screen.
- MAME 0.242 reference with the same scripted inputs: continuous, only short
  transition silences. Reference RMS 2850 vs fixed native 2813.
- --verify: 9,658,521 blocks, 0 mismatches; 19,000-frame native coverage
  100.00% (114 interpreted instructions).
- tests/audio_regression.py passes native and interpreter on this build and
  fails the old interpreter build at the 12-second 96-107 s gap.

QUICK START
-----------
1. Put your prepared ROM directory at:
     /home/deck/.local/share/garou/roms
   (the build auto-discovers it), or set GAROU_ROM_DIR, or put a roms folder
   next to the AppImage, or pass --rom-path.

   The folder must contain 253-pd.bin, 253-c1..c8, 253-s1.s1, 253-m1.m1,
   253-v1..v4 plus sp-s2.sp1, sfix.sfix, sm1.sm1 and 000-lo.lo. The in-app
   folder picker can also take the original encrypted set (253-ep1.p1) and
   decrypt it once into ~/.local/share/garou/roms.

2. Mark the AppImage executable:
     chmod +x Garou-AudioFixed-x86_64.AppImage

3. Desktop Mode: double-click it, or run it from a terminal.
   Gaming Mode: add it as a non-Steam game and launch it.

The AppRun script adds --fullscreen automatically. Pass --scale 2 or
--headless to override; F11 toggles fullscreen and Esc quits. Use
--select-rom to force the ROM folder picker; --rom-path bypasses it.

CONTROLS
--------
  D-pad / left stick         move
  A B X Y (Xbox layout)      Neo Geo A B C D
  Back                       insert coin
  Start                      player 1 start
  Left shoulder              select
  Keyboard: arrows, Z X C V = A B C D, 5 = coin, 1 = start, 3 = select,
  9 = service, F2 = test, F11 = fullscreen, Esc = quit.

The toolkit uses SDL2's GameController mapping. A pad SDL does not recognise
can be described with SDL_GAMECONTROLLERCONFIG. A standard Xbox-layout pad
works without extra configuration.

DISPLAY
-------
Native 320x224 presented at 4:3 in fullscreen (1280x800 -> 1066x800
pillarboxed), not stretched to 16:10. Internal refresh is the MVS 59.1856 Hz
and frames are paced to it.

LICENSE
-------
Original project code is PolyForm Noncommercial 1.0.0. The vendored
neogeorecomp toolkit is MIT; Musashi/z80 are MIT; ymfm and the MAME
decryption material are BSD-3-Clause; SDL2 is zlib. See the LICENSES folder
inside the AppImage.

No ROMs or BIOS are included. This AppImage contains code generated from
SNK's ROM data; use it only with ROMs you are legally allowed to use. Garou:
Mark of the Wolves and Neo Geo are trademarks of SNK Corporation. This
project is not affiliated with or endorsed by SNK.

LIMITATIONS
-----------
Physical Steam Deck LCD/OLED hardware, Steam Input and Gaming Mode were not
available in this environment, so those are not claimed as tested. The
subjective audio mix was not auditioned; continuity and level are measured.
