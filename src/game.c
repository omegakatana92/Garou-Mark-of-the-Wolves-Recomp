/*
 * game.c - Garou: Mark of the Wolves (MAME set "garou"), the verified ROM
 * layout shared by the player and the recompiler.
 *
 * The supplied set is SNK's original encrypted MVS release.  tools/decrypt.c
 * turns it into the files named here at build time, in the build directory:
 *
 *   253-pd.bin      9 MB program: 1 MB fixed bank + 8 MB banked image
 *   253-c1..c8      8 MB each, NEO-CMC42 decrypted
 *   253-s1.s1       512 KB fixed layer extracted from the end of the C data
 *
 * The decrypted program keeps the original NEO-SMA layout.  The cartridge
 * chooses a 64 KB-granular window from an unscrambled 6-bit selector through
 * the NEO-SMA mapper at $2FFFC0; the selector bits and the offset table come
 * from MAME's sma_prot_device::garou_bank_base().  The same chip returns
 * $9A37 at $2FE446 and its 16-bit LFSR at $2FFFCC and $2FFFF0.
 */
#include "game.h"

/* garou_bank_base() offsets, relative to the start of the 8 MB banked image.
 * MAME stores them as 0x100000 + offset in the full cartridge region. */
static const uint32_t k_garou_bank_offsets[64] = {
    0x000000, 0x100000, 0x200000, 0x300000, /* 00 */
    0x280000, 0x380000, 0x2D0000, 0x3D0000, /* 04 */
    0x2F0000, 0x3F0000, 0x400000, 0x500000, /* 08 */
    0x420000, 0x520000, 0x440000, 0x540000, /* 12 */
    0x498000, 0x598000, 0x4A0000, 0x5A0000, /* 16 */
    0x4A8000, 0x5A8000, 0x4B0000, 0x5B0000, /* 20 */
    0x4B8000, 0x5B8000, 0x4C0000, 0x5C0000, /* 24 */
    0x4C8000, 0x5C8000, 0x4D0000, 0x5D0000, /* 28 */
    0x458000, 0x558000, 0x460000, 0x560000, /* 32 */
    0x468000, 0x568000, 0x470000, 0x570000, /* 36 */
    0x478000, 0x578000, 0x480000, 0x580000, /* 40 */
    0x488000, 0x588000, 0x490000, 0x590000, /* 44 */
    0x5D0000, 0x5D8000, 0x5E0000, 0x5E8000, /* 48 */
    0x5F0000, 0x5F8000, 0x600000,           /* 52; the rest are unused */
    0, 0, 0, 0, 0, 0, 0, 0, 0,
};

static const ng_bank_map_t k_garou_bank_map = {
    .offsets = k_garou_bank_offsets,
    .count = 64,
    .bits = {12, 14, 6, 7, 9, 5},   /* bitswap<6>(sel, 12,14,6,7,9,5) */
};

const ng_game_t k_garou = {
    .name = "garou",
    .title = "Garou - Mark of the Wolves",
    .prom_size = 0x900000,          /* 1 MB fixed + 8 MB banked */
    .prom = {
        {"253-pd.bin", 0x000000, 0x000000, 0x900000, 0},  /* decrypted image */
    },
    .crom_size = 0x4000000,         /* 64 MB after pairing */
    .crom_pairs = {
        {"253-c1.c1", "253-c2.c2"},
        {"253-c3.c3", "253-c4.c4"},
        {"253-c5.c5", "253-c6.c6"},
        {"253-c7.c7", "253-c8.c8"},
    },
    .srom = "253-s1.s1",            /* 512 KB, extracted from the C data */
    .m1 = "253-m1.m1",              /* plain Z80 program on this board */
    .vrom = {"253-v1.v1", "253-v2.v2", "253-v3.v3", "253-v4.v4"},
    .fix_bank_type = NG_FIX_BANK_GAROU,
    .bank_map = &k_garou_bank_map,
    .bank_write_addr = 0x2FFFC0,    /* NEO-SMA bank select */
    .prot_read_addr = 0x2FE446,     /* NEO-SMA protection read */
    .prot_value = 0x9A37,
    .rng_read_addr = {0x2FFFCC, 0x2FFFF0},  /* NEO-SMA LFSR */
    .rng_seed = 0x2345,
};
