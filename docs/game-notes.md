# Game notes: Garou - Mark of the Wolves

## ROM set

The supplied `garou.zip` is the MAME `garou` set (MVS later revision,
NGM-2530). It is SNK's original encrypted release:

| Role | Files | Size | Encoding |
|---|---|---|---|
| P (68k program) | `253-ep1.p1` .. `253-ep4.p4` | 4 x 2 MB | NEO-SMA, `ROM_LOAD16_WORD_SWAP` |
| SMA data | `kf.neo-sma` | 256 KB | stored in the custom chip, loaded at `$0C0000` |
| C (sprites) | `253-c1.c1` .. `253-c8.c8` | 8 x 8 MB | NEO-CMC42 |
| S (fixed layer) | none | 512 KB derived | `sfix_decrypt()` from the end of the C data |
| M (Z80 program) | `253-m1.m1` | 256 KB | plain |
| V (ADPCM) | `253-v1.v1` .. `253-v4.v4` | 4 x 4 MB | plain |
| BIOS | `sp-s2.sp1`, `sfix.sfix`, `sm1.sm1`, `000-lo.lo` | | from `neogeo.zip` |

The encrypted board has no separate S ROM. MAME fills the 512 KB fixed region
from the last 512 KB of the decrypted sprite data.

## Boot sequence observed

1. BIOS memory test and eye-catcher (`SNK`, about frame 600).
2. Cartridge header check and game init.
3. Intro art and `INSERT COIN(S)` (about frame 1200).
4. Title / attract loop (about frame 2500 onward; the attract match starts
   around frame 3000).
5. Coin (frame 900) + Start (frame 1000): HOW TO PLAY, T.O.P. SYSTEM,
   PLAYER SELECT.
6. Character select has a 5-count timer. Press A to lock Rock; the CPU picks
   Hotaru. T.O.P. SET follows, then the stage.
7. Gameplay, HUD, win/loss sequence.

## Program layout

`tools/decrypt.c` builds a 9 MB image:

```
$000000-$0FFFFF  1 MB fixed bank (768 KB relocated code + 256 KB kf.neo-sma)
$100000-$8FFFFF  8 MB banked image (NEO-SMA permutations applied)
```

MAME's `garou_decrypt_68k()` does, in order:

1. data-line bitswap over the 8 MB banked image;
2. fixed 768 KB relocation from `$710000` with a 19-bit address bitswap;
3. 32 KB block address permutation over the banked image.

The `NEO-GEO` signature at `$100` and the reset vectors are checked after
decryption. `SP=0010F300`, `PC=00C00402` (the BIOS vector page).

## Banking

The NEO-SMA mapper selects a 64 KB-granular window through `$2FFFC0`. The six
selector bits are unscrambled with `bitswap<6>(sel, 12,14,6,7,9,5)` and index
the 64-entry `garou_bank_base()` table. The runtime's recompiler describes the
first MB of the banked image; other windows are interpreted for correctness
(see the coverage section of the README).

## Protection

Garou's cart exposes:

* `$2FE446` -> `$9A37` (protection read);
* `$2FFFCC` and `$2FFFF0` -> the NEO-SMA 16-bit LFSR, seed `$2345`.

The runtime models all three generically. The MVS watchdog reset is also
modelled; late games use a deliberate long loop and expect the board to reset.

## Fixed layer

Garou's 512 KB S ROM is banked by a 32-line table. For each visible line the
renderer walks `$7500`/`$7580`; a marker `0200` / `ffxx` switches the 4-bank
window. The tile code is `map_code + 0x1000 * (line_bank ^ 3)`. This is MAME's
`FIX_BANKTYPE_GAROU`, distinct from the KOF2000 six-column layout.

## Audio

The M1 is not encrypted on this board. The Z80 driver uses the standard NEO-ZMC
banking and talks to the YM2610. The 16 MB V region is four 4 MB samples
concatenated. The runtime produces non-silent stereo output; subjective mix
quality was not auditioned.
