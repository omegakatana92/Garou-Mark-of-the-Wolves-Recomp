Garou: Mark of the Wolves - native static recompilation (Windows x64)
=====================================================================

This folder contains the recompiled player and its runtime dependencies.
It does NOT contain game ROMs or the Neo Geo BIOS.

Quick start
-----------
1. Prepare your own legally obtained MAME dumps:

   - garou.zip   (the "garou" set: 253-ep1..ep4, 253-c1..c8, 253-m1,
                  253-v1..v4, kf.neo-sma)
   - neogeo.zip  (the system ROM set: sp-s2.sp1, sfix.sfix, sm1.sm1,
                  000-lo.lo)

   Run the helper from this folder:

       powershell -ExecutionPolicy Bypass -File .\Prepare-ROMs.ps1

   It asks for the two zips, extracts them into roms-src\, runs
   garou_decrypt.exe, and writes the runtime-ready set into roms\.

   To do it by hand:

       mkdir roms-src
       tar -xf garou.zip -C roms-src
       tar -xf neogeo.zip -C roms-src
       .\garou_decrypt.exe --rom-path roms-src --out roms

2. Run:

       Garou.exe --rom-path roms

   The BIOS is the real sp-s2.sp1 MVS system ROM. There is no BIOS HLE.

Controls
--------
  Arrow keys              move
  Z X C V                 Neo Geo A B C D
  5                       insert coin
  1                       player 1 start
  2                       player 2 start
  3                       select
  9                       service credit
  F2                      test switch
  F11                     toggle fullscreen
  Esc                     quit

  SDL2 gamepads: D-pad or left stick; A B X Y = A B C D; Start = start;
  Back = coin; left shoulder = select.

Options
-------
  --rom-path DIR          game ROM files (default: roms)
  --bios-path DIR         system ROM files (default: the ROM path)
  --bios FILE             system ROM (default: sp-s2.sp1)
  --headless              no window; run as fast as possible
  --frames N              stop after N frames
  --screenshot N:FILE     save frame N as PNG (repeatable)
  --input FILE            scripted input
  --wav FILE              write audio to a WAV file
  --interp                run everything in the interpreter
  --verify                replay native blocks in Musashi and compare
  --dump-misses FILE      write interpreted entry points
  --scale N               window scale (default 3)
  --fullscreen            fullscreen, 4:3 pillarboxed
  --mute                  no sound from the window

Status and limitations
----------------------
  * Windows x64 Release build, MSVC 19.51 (VS 18 2026), SDL2 2.32.10.
  * Boot, coin/start, menus, player select and a full match were tested.
  * Native coverage: 100.00% of a 19,000-frame scripted session; 92.38% of a
    6,000-frame no-input attract session (bank-window fallback to Musashi).
  * --verify: 9,658,446 blocks, 0 mismatches over 8,000 gameplay frames.
  * Audio signal is present; subjective mix quality was not auditioned.
  * Physical Steam Deck/gamepad not tested; memory cards are not implemented.

Licensing
---------
  The project code is licensed under PolyForm Noncommercial 1.0.0; see
  LICENSES\LICENSE-STATUS.txt and LICENSES\LICENSE.txt. The bundled
  neogeorecomp toolkit remains MIT; Musashi/z80 are MIT; ymfm and the MAME
  decryption material are BSD-3-Clause; SDL2 is zlib. See
  LICENSES\THIRD_PARTY_NOTICES.txt.

  Garou: Mark of the Wolves and Neo Geo are trademarks of SNK Corporation.
  This project is not affiliated with or endorsed by SNK. You are responsible
  for using ROM images you are legally allowed to use.
