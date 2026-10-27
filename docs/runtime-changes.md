# Runtime changes in the vendored toolkit

`neogeorecomp/` is the upstream MIT toolkit at v0.2.0 (commit in
`toolkit.ref`), copied from the owner's existing v0.2.0 working tree. That
tree already contained generic changes made for earlier late-cart ports; this
project adds the NEO-SMA RNG and Garou's fixed-layer layout. None of the
changes contain a Garou address, title string or title-specific branch: the
game supplies its own data through `ng_game_t`.

## Changes used by this port

| File | Change | Why it is generic |
|---|---|---|
| `include/neogeorecomp/neogeo.h` | `ng_bank_map_t`, `bank_write_addr`, `prot_read_addr`, `prot_value`, `rng_read_addr[2]`, `rng_seed` | Any cart can describe its mapper and protection ports |
| `include/neogeorecomp/bus.h` | Same fields in `ng_board_t`; `ng_bank_base` exported | The bus needs the cart's mapper data |
| `src/bus.c` | Non-linear bank table, bank-select port, protection read, NEO-SMA LFSR | NEO-SMA / CMC carts share this behaviour |
| `src/exec.c` | Count native and interpreted instructions; refuse native bank-0 code when the window is at another base | Correctness: the same 68k address can mean different bytes |
| `src/video.c` | `NG_FIX_BANK_GAROU` / `NG_FIX_BANK_KOF2000` fixed-layer bank tables | Late carts bank the 512 KB S ROM; two layouts exist |
| `src/neogeo.c`, `src/io.c` | MVS watchdog reset after 1,622,015 68k clocks | Hardware behaviour; late games use it as a protection check |
| `neogeorecomp/src/audio.c` | Correct M1 bank file offset/mask; sync the Z80 before sample generation | NEO-ZMC banking is generic; every M1-banked game is affected |
| `src/neogeo.c`, `src/window_sdl.c`, `src/platform.c` | `--fullscreen`, 4:3 pillarboxing | Generic presentation option |
| `src/neogeo.c`, `src/ng_internal.h` | Board initializer carries the new cart fields | Plumbing |
| `CMakeLists.txt` (toolkit) | `/O1` for generated giant functions on MSVC | Build-time workaround, no behaviour change |

## Garou-specific data, kept out of the toolkit

`src/game.c` contains only the MAME `garou` ROM layout:

* the 64-entry NEO-SMA bank offset table and selector bit order,
* the protection read address/value,
* the RNG latch addresses and seed,
* the 512 KB S ROM and Garou fixed-layer selector.

The same values are visible in MAME's `prot_sma.cpp` / `sma.cpp`. They are
cart data, not runtime policy.

## Fixed-layer enum

The upstream v0.2.0 tree only had `NG_FIX_BANK_NONE` and a single late-cart
layout. The vendored copy now uses MAME's numbering:

```
NG_FIX_BANK_NONE   = 0
NG_FIX_BANK_GAROU  = 1   (Garou / MSlug 3: 32-line table at $7500/$7580)
NG_FIX_BANK_KOF2000 = 2  (six columns of 2-bit bank codes)
```

`video.c` implements both. The earlier KOF2000/MSLUG3 projects in this
toolkit tree keep their own toolkit copies, so their numbering is unaffected.

## Audio bank mapping

The Z80's four switchable windows are selected by reading ports `$08-$0B`;
the high byte of the port address is the bank number.  MAME loads the M1
into an audio region of `M1 size + 0x10000` bytes with a second copy at
`0x10000`, and computes each bank entry as:

```
0x10000 + ((bank << (11 + window)) & ((region_size - 0x10000 - 1) & 0x3ffff))
```

The file offset is therefore the masked value alone.  The earlier code added
another `0x10000` when reading the file, so every banked window fetched data
64 KB too high.  The fixed bank (driver code) was unaffected, which is why
the driver ran but its music data was corrupt.  `audio.c` now uses
`(M1 size - 1) & 0x3ffff` and no extra base, and calls `audio_sync()` before
generating samples so the sequencer keeps running when the 68000 is not
polling a sound port.

`tests/audio_regression.py` guards the behaviour: a 11,000-frame session must
not contain a silent run longer than 3 seconds after boot.  It passes native
and interpreter on the fixed build and fails the old interpreter build at the
12-second 96-107 s gap.

## Verification of the changes

* The NEO-SMA RNG matches MAME's `random_r()` bit-for-bit.
* The bank table and bit order match MAME's `garou_bank_base()`.
* The decryption matches MAME's `garou_decrypt_68k()` and
  `cmc42_gfx_decrypt(..., 0x06)`.
* `--verify` replays every native block in Musashi against the same bus
  values; 9.66M blocks over gameplay, 0 mismatches.
* No-input native and interpreter runs produce byte-identical frames.
