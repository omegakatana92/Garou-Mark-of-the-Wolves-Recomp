# Third-party notices

This project is a game host and recompilation setup for **neogeorecomp**.
The following components keep their own licenses. Nothing here changes them.

## neogeorecomp (MIT)

`neogeorecomp/` is the upstream static-recompilation toolkit.

    Copyright (c) 2026 sp00nznet

    MIT License. The full text is in neogeorecomp/LICENSE.

The vendored copy started from the v0.2.0 tree (upstream commit
`af96b2b4b39b42b392b17bcaa6a74a5f27088202`, see `toolkit.ref`) and carries the
generic changes listed in `docs/runtime-changes.md`. Those changes are
released under the same MIT terms as the toolkit. The upstream `NOTICE` and
`LICENSES/` directory are preserved.

## Musashi (MIT)

`neogeorecomp/ext/musashi/` - Motorola 68000 interpreter, the fallback and
the `--verify` oracle.

    Copyright (c) 1998-2001 Karl Stenerud

Full text: `neogeorecomp/LICENSES/Musashi.txt`.

## superzazu/z80 (MIT)

`neogeorecomp/ext/z80/` - Zilog Z80 core used as the sound CPU.

    Copyright (c) 2019 Nicolas Allemand

Full text: `neogeorecomp/LICENSES/z80.txt`.

## ymfm (BSD-3-Clause)

`neogeorecomp/ext/ymfm/` - Yamaha FM/ADPCM synthesis, used for the YM2610.

    Copyright (c) 2021 Aaron Giles
    All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its
   contributors may be used to endorse or promote products derived from this
   software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.

Full text: `neogeorecomp/LICENSES/ymfm.txt`.

## MAME Neo Geo decryption material (BSD-3-Clause)

`tools/prot_cmc.cpp`, `tools/prot_sma.cpp` and `tools/tables.h` are derived
from MAME's Neo Geo driver/protection devices:

    Copyright (c) S. Smith, David Haywood, Fabio Priuli, Razoola, Mr.K
    BSD-3-Clause.

The files retain their `license:BSD-3-Clause` headers. The generated tables
in `tools/tables.h` are a mechanical extraction of the CMC42/CMC50 tables in
`tools/prot_cmc.cpp`. The BSD-3-Clause terms are reproduced in the ymfm
section above. These files are used at build time to convert the user's own
encrypted ROMs; the decrypted output is not distributed.

## SDL2 (zlib)

The Windows package includes `SDL2.dll` and the AppImage bundles
`libSDL2-2.0.so.0`.

    Copyright (C) 1997-2024 Sam Lantinga <slouken@libsdl.org>

This software is provided 'as-is', without any express or implied warranty.
In no event will the authors be held liable for any damages arising from the
use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software in a
   product, an acknowledgment in the product documentation would be
   appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.

## AppImage runtime

The AppImage is produced with the AppImage type-2 runtime and appimagetool
(AppImage project, MIT). Those tools are build/packaging tools; their source
and licenses are at <https://github.com/AppImage>.

## Game data

No SNK ROM data, BIOS images, decrypted ROMs, recompiled C, entry-point lists
or other ROM-derived artifacts are distributed with this project. Gameplay
screenshots are not committed to the public repository; they remain SNK's
copyrighted imagery.

Garou: Mark of the Wolves and Neo Geo are trademarks of SNK Corporation.
This project is not affiliated with or endorsed by SNK.
