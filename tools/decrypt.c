/*
 * decrypt.c - Garou: Mark of the Wolves (MAME set "garou") ROM preparation.
 *
 * The supplied set is SNK's original encrypted MVS release:
 *   - program: NEO-SMA protected (253-ep1..ep4 + kf.neo-sma)
 *   - graphics: NEO-CMC42 protected (253-c1..c8), S data at the end of C
 *   - audio: 253-m1.m1 is plain on this board; 253-v1..v4 are plain
 *
 * This build-time tool reads those files and writes a runtime-ready set:
 *
 *   253-pd.bin    9 MB program image (1 MB fixed + 8 MB bank, decrypted)
 *   253-c1..c8    CMC42-decrypted sprite ROMs
 *   253-s1.s1     512 KB fixed-layer ROM extracted from the C data
 *   plus the M1, V ROMs and the MVS BIOS files, copied unchanged.
 *
 * The decryption algorithms and tables are ported from MAME's Neo Geo
 * driver (BSD-3-Clause; S. Smith, D. Haywood, F. Priuli, Razoola, Mr.K).
 * See tools/prot_cmc.cpp and tools/prot_sma.cpp.  No ROM data is written
 * outside the output directory.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#ifdef _WIN32
#include <direct.h>
#define GAROU_MKDIR(p) _mkdir(p)
#else
#include <sys/stat.h>
#define GAROU_MKDIR(p) mkdir((p), 0777)
#endif
#include "tables.h"

#define PROM_FIXED   0x100000u
#define PROM_BANK    0x800000u
#define PROM_SIZE    (PROM_FIXED + PROM_BANK)     /* 9 MB decrypted image */
#define CROM_SIZE    0x4000000u                   /* 64 MB interleaved */
#define SROM_SIZE    0x80000u                     /* 512 KB fixed layer */

/* MAME's bitswap<N>(value, b0, b1, ...): output bit N-1-i is input bit b[i]. */
static uint32_t bitswap(uint32_t val, const uint8_t *bits, int n) {
    uint32_t r = 0;
    for (int i = 0; i < n; i++)
        r |= ((val >> bits[i]) & 1u) << (n - 1 - i);
    return r;
}

static uint8_t *read_file(const char *dir, const char *name, size_t *size) {
    char path[1024];
    snprintf(path, sizeof(path), "%s/%s", dir, name);
    FILE *f = fopen(path, "rb");
    if (!f) { fprintf(stderr, "decrypt: cannot open %s\n", path); return NULL; }
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (n <= 0) { fclose(f); return NULL; }
    uint8_t *buf = (uint8_t *)malloc((size_t)n);
    if (!buf || fread(buf, 1, (size_t)n, f) != (size_t)n) {
        fclose(f); free(buf); return NULL;
    }
    fclose(f);
    *size = (size_t)n;
    return buf;
}

static int write_file(const char *dir, const char *name, const uint8_t *data, size_t size) {
    char path[1024];
    snprintf(path, sizeof(path), "%s/%s", dir, name);
    FILE *f = fopen(path, "wb");
    if (!f) { fprintf(stderr, "decrypt: cannot write %s\n", path); return -1; }
    if (fwrite(data, 1, size, f) != size) { fclose(f); return -1; }
    fclose(f);
    return 0;
}

static int copy_file(const char *dir, const char *outdir, const char *name) {
    size_t size;
    uint8_t *d = read_file(dir, name, &size);
    if (!d) return -1;
    int rc = write_file(outdir, name, d, size);
    free(d);
    return rc;
}

/* MAME loads P ROMs with ROM_LOAD16_WORD_SWAP: the file bytes are swapped
 * into the 68000's big-endian word order. */
static void copy_swapped(uint8_t *dst, const uint8_t *src, size_t n) {
    for (size_t i = 0; i + 1 < n; i += 2) {
        dst[i] = src[i + 1];
        dst[i + 1] = src[i];
    }
}

static uint16_t rd16(const uint8_t *p) { return (uint16_t)((p[0] << 8) | p[1]); }
static void wr16(uint8_t *p, uint16_t v) { p[0] = (uint8_t)(v >> 8); p[1] = (uint8_t)v; }

static int decrypt_prom(const char *dir, const char *outdir) {
    static const char *names[4] = {
        "253-ep1.p1", "253-ep2.p2", "253-ep3.p3", "253-ep4.p4",
    };
    static const uint8_t data_bits[16]  = {13,12,14,10,8,2,3,1,5,9,11,4,15,0,6,7};
    static const uint8_t fixed_bits[19] = {18,4,5,16,14,7,9,6,13,17,15,3,1,2,12,11,8,10,0};
    static const uint8_t bank_bits[14]  = {9,4,8,3,13,6,2,7,0,12,1,11,10,5};

    uint8_t *rom[4] = {0};
    uint8_t *sma = NULL;
    size_t n = 0;
    for (int i = 0; i < 4; i++) {
        rom[i] = read_file(dir, names[i], &n);
        if (!rom[i] || n != 0x200000) {
            fprintf(stderr, "decrypt: %s missing or not 2 MB\n", names[i]);
            return -1;
        }
    }
    sma = read_file(dir, "kf.neo-sma", &n);
    if (!sma || n != 0x40000) {
        fprintf(stderr, "decrypt: kf.neo-sma missing or not 256 KB\n");
        return -1;
    }

    uint8_t *region = (uint8_t *)calloc(1, PROM_SIZE);
    if (!region) return -1;
    for (int i = 0; i < 4; i++) {
        copy_swapped(region + 0x100000 + (uint32_t)i * 0x200000, rom[i], 0x200000);
        free(rom[i]);
    }
    copy_swapped(region + 0x0C0000, sma, 0x40000);
    free(sma);

    /* MAME sma_prot_device::garou_decrypt_68k(). */
    for (uint32_t i = 0; i < 0x800000 / 2; i++) {
        uint8_t *p = region + 0x100000 + 2 * i;
        wr16(p, (uint16_t)bitswap(rd16(p), data_bits, 16));
    }

    /* Relocate the fixed 768 KB of code out of the banked image.  The
     * 256 KB SMA data at $0C0000..$0FFFFF is left in place. */
    for (uint32_t i = 0; i < 0x0C0000 / 2; i++) {
        const uint8_t *src = region + 0x710000 + 2 * bitswap(i, fixed_bits, 19);
        uint8_t *dst = region + 2 * i;
        dst[0] = src[0]; dst[1] = src[1];
    }

    /* Address swap for the banked part, in 32 KB blocks. */
    uint8_t *buffer = (uint8_t *)malloc(0x8000);
    if (!buffer) { free(region); return -1; }
    for (uint32_t base = 0; base < 0x800000; base += 0x8000) {
        memcpy(buffer, region + 0x100000 + base, 0x8000);
        for (uint32_t j = 0; j < 0x8000 / 2; j++) {
            const uint8_t *src = buffer + 2 * bitswap(j, bank_bits, 14);
            uint8_t *dst = region + 0x100000 + base + 2 * j;
            dst[0] = src[0]; dst[1] = src[1];
        }
    }
    free(buffer);

    /* Sanity: the fixed bank must now carry SNK's cartridge header. */
    if (memcmp(region + 0x100, "NEO-GEO", 7) != 0) {
        fprintf(stderr, "decrypt: error: no NEO-GEO signature at $100 after P ROM decryption\n");
        free(region);
        return -1;
    }
    fprintf(stderr, "decrypt: P ROM -> 253-pd.bin (SP=%08X PC=%08X)\n",
            (unsigned)((region[0] << 24) | (region[1] << 16) | (region[2] << 8) | region[3]),
            (unsigned)((region[4] << 24) | (region[5] << 16) | (region[6] << 8) | region[7]));

    int rc = write_file(outdir, "253-pd.bin", region, PROM_SIZE);
    free(region);
    return rc;
}

/* ---- NEO-CMC42 graphics ---- */

static void gfx_byte(uint8_t *r0, uint8_t *r1, uint8_t c0, uint8_t c1,
                     const uint8_t *t0hi, const uint8_t *t0lo, const uint8_t *t1,
                     uint32_t base, int invert) {
    uint8_t tmp  = t1[(base & 0xff) ^ kof99_address_0_7_xor[(base >> 8) & 0xff]];
    uint8_t xor0 = (uint8_t)((t0hi[(base >> 8) & 0xff] & 0xfe) | (tmp & 0x01));
    uint8_t xor1 = (uint8_t)((tmp & 0xfe) | (t0lo[(base >> 8) & 0xff] & 0x01));
    if (invert) { *r0 = (uint8_t)(c1 ^ xor0); *r1 = (uint8_t)(c0 ^ xor1); }
    else        { *r0 = (uint8_t)(c0 ^ xor0); *r1 = (uint8_t)(c1 ^ xor1); }
}

static void gfx_decrypt(uint8_t *rom, uint32_t rom_size, uint32_t extra_xor) {
    uint8_t *buf = (uint8_t *)malloc(rom_size);
    if (!buf) return;
    for (uint32_t rpos = 0; rpos < rom_size / 4; rpos++) {
        gfx_byte(&buf[4*rpos], &buf[4*rpos+3], rom[4*rpos], rom[4*rpos+3],
                 kof99_type0_t03, kof99_type0_t12, kof99_type1_t03, rpos,
                 (int)((rpos >> 8) & 1));
        gfx_byte(&buf[4*rpos+1], &buf[4*rpos+2], rom[4*rpos+1], rom[4*rpos+2],
                 kof99_type0_t12, kof99_type0_t03, kof99_type1_t12, rpos,
                 (int)(((rpos >> 16) ^ kof99_address_16_23_xor2[(rpos >> 8) & 0xff]) & 1));
    }
    for (uint32_t rpos = 0; rpos < rom_size / 4; rpos++) {
        uint32_t baser = rpos ^ extra_xor;
        baser ^= (uint32_t)kof99_address_8_15_xor1[(baser >> 16) & 0xff] << 8;
        baser ^= (uint32_t)kof99_address_8_15_xor2[baser & 0xff] << 8;
        baser ^= (uint32_t)kof99_address_16_23_xor1[baser & 0xff] << 16;
        baser ^= (uint32_t)kof99_address_16_23_xor2[(baser >> 8) & 0xff] << 16;
        baser ^= kof99_address_0_7_xor[(baser >> 8) & 0xff];
        baser &= (rom_size / 4) - 1;
        rom[4*rpos]     = buf[4*baser];
        rom[4*rpos + 1] = buf[4*baser + 1];
        rom[4*rpos + 2] = buf[4*baser + 2];
        rom[4*rpos + 3] = buf[4*baser + 3];
    }
    free(buf);
}

static void sfix_extract(const uint8_t *rom, uint32_t rom_size,
                         uint8_t *fixed, uint32_t fixed_size) {
    const uint8_t *src = rom + rom_size - fixed_size;
    for (uint32_t i = 0; i < fixed_size; i++)
        fixed[i] = src[(i & ~0x1fu) + ((i & 7) << 2) + ((~i & 8) >> 2) + ((i & 0x10) >> 4)];
}

static int decrypt_crom(const char *dir, const char *outdir) {
    static const char *pairs[4][2] = {
        {"253-c1.c1", "253-c2.c2"},
        {"253-c3.c3", "253-c4.c4"},
        {"253-c5.c5", "253-c6.c6"},
        {"253-c7.c7", "253-c8.c8"},
    };
    size_t csize = 0;
    uint8_t *even[4] = {0}, *odd[4] = {0};
    for (int i = 0; i < 4; i++) {
        even[i] = read_file(dir, pairs[i][0], &csize);
        if (!even[i] || csize != 0x800000) {
            fprintf(stderr, "decrypt: %s missing or not 8 MB\n", pairs[i][0]);
            return -1;
        }
        odd[i] = read_file(dir, pairs[i][1], &csize);
        if (!odd[i] || csize != 0x800000) {
            fprintf(stderr, "decrypt: %s missing or not 8 MB\n", pairs[i][1]);
            return -1;
        }
    }
    uint32_t total = 0x800000u * 2 * 4;
    if (total != CROM_SIZE) return -1;
    uint8_t *crom = (uint8_t *)malloc(total);
    if (!crom) return -1;
    for (int i = 0; i < 4; i++) {
        uint32_t base = 0x800000u * 2 * (uint32_t)i;
        for (uint32_t k = 0; k < 0x800000; k++) {
            crom[base + 2*k]     = even[i][k];
            crom[base + 2*k + 1] = odd[i][k];
        }
    }
    gfx_decrypt(crom, total, 0x06);   /* GAROU_GFX_KEY */

    uint8_t *srom = (uint8_t *)malloc(SROM_SIZE);
    if (!srom) { free(crom); return -1; }
    sfix_extract(crom, total, srom, SROM_SIZE);
    if (write_file(outdir, "253-s1.s1", srom, SROM_SIZE)) { free(srom); free(crom); return -1; }
    free(srom);
    fprintf(stderr, "decrypt: S ROM -> 253-s1.s1 (0x%X bytes)\n", SROM_SIZE);

    for (int i = 0; i < 4; i++) {
        uint32_t base = 0x800000u * 2 * (uint32_t)i;
        for (uint32_t k = 0; k < 0x800000; k++) {
            even[i][k] = crom[base + 2*k];
            odd[i][k]  = crom[base + 2*k + 1];
        }
        if (write_file(outdir, pairs[i][0], even[i], 0x800000)) return -1;
        if (write_file(outdir, pairs[i][1], odd[i],  0x800000)) return -1;
        free(even[i]); free(odd[i]);
    }
    free(crom);
    fprintf(stderr, "decrypt: C ROMs -> 253-c1..c8 (8 MB each)\n");
    return 0;
}

int main(int argc, char **argv) {
    const char *romdir = "roms-src", *outdir = "roms";
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--rom-path") && i + 1 < argc) romdir = argv[++i];
        else if (!strcmp(argv[i], "--out") && i + 1 < argc) outdir = argv[++i];
        else if (!strcmp(argv[i], "--help")) {
            fprintf(stderr, "usage: garou_decrypt [--rom-path DIR] [--out DIR]\n");
            return 0;
        }
    }
    GAROU_MKDIR(outdir);
    if (decrypt_prom(romdir, outdir)) return 1;
    if (decrypt_crom(romdir, outdir)) return 1;
    const char *copy[] = {
        "253-m1.m1",
        "253-v1.v1", "253-v2.v2", "253-v3.v3", "253-v4.v4",
        "sp-s2.sp1", "sfix.sfix", "sm1.sm1", "000-lo.lo",
        NULL,
    };
    for (int i = 0; copy[i]; i++)
        if (copy_file(romdir, outdir, copy[i])) return 1;
    fprintf(stderr, "decrypt: wrote a complete ROM set to %s\n", outdir);
    return 0;
}
