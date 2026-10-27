Garou: Mark of the Wolves - native static recompilation
========================================================

This AppImage contains the recompiled game, the SDL2 window/audio layer and
the build-time ROM decrypt tool.  It does not contain game ROMs or the Neo Geo
BIOS: supply your own legally obtained dump.

Quick start
-----------
1. Put your MAME "garou" set and "neogeo" set in a folder, unzipped:

     mkdir -p roms
     unzip garou.zip -d roms
     unzip neogeo.zip -d roms

   The AppRun script finds a "roms" directory next to the AppImage, or a
   folder named by GAROU_ROM_DIR.  If it sees the encrypted 253-ep1.p1 set,
   it decrypts it once into ~/.local/share/garou/roms and launches from
   there.  To prepare the files yourself:

     ./Garou_Mark_of_The_Wolves-SteamDeck-x86_64.AppImage --appimage-extract
     ./squashfs-root/usr/bin/garou_decrypt --rom-path roms --out roms

2. Run the AppImage.  It starts in fullscreen with a 4:3 pillarboxed image.

If no ROM folder is found, the player opens a folder picker (zenity,
kdialog or yad; a terminal prompt is the fallback).  Select either the
decrypted folder (contains 253-pd.bin) or the original encrypted set
(contains 253-ep1.p1); the bundled garou_decrypt prepares a cached copy in
~/.local/share/garou/roms.  Use --select-rom to force the picker, or
--rom-path to bypass it.  --headless never opens a dialog.  In Gaming Mode
without a dialog helper, put the roms folder next to the AppImage or set
GAROU_ROM_DIR in the launch options.

Steam Deck
----------
Copy the AppImage and a roms folder to the Deck, mark the AppImage executable
(chmod +x), and add it as a non-Steam game.  It runs in Game Mode and Desktop
Mode.  If your ROMs live somewhere else, set GAROU_ROM_DIR in the game's
launch options.  The AppImage is built on Ubuntu 22.04 (glibc 2.35), so it
also runs on SteamOS 3.

Controls
--------
  Arrows / left stick    move
  Z X C V or pad A B X Y Neo Geo A B C D
  5 or pad Back          insert coin
  1 or pad Start         start
  3 or pad left shoulder select
  9                      service
  F2                     test switch
  F11                    toggle fullscreen
  Esc                    quit

Notes
-----
* The MVS BIOS is the real sp-s2.sp1 system ROM; there is no BIOS HLE.
* The game's 68000 code is statically recompiled.  Code reached through a
  swapped bank window uses the bundled Musashi interpreter; the measured
  share is reported in the project README.
* Audio is produced by the real Z80 driver and the YM2610 model.  The M1 bank
  mapping was corrected in this build; 19,000-frame native and interpreter
  captures are continuous through character select and the early match.  The
  subjective mix was not auditioned.
