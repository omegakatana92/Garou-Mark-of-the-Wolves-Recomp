/*
 * main.c - the Garou: Mark of the Wolves player.
 *
 * The hardware model, the interpreter fallback and the window all live in
 * neogeorecomp.  This file hands the runtime the ROM layout and the table of
 * routines that garou_recomp generated from the user's own dump at build
 * time.  Without that table the runtime interprets.
 */
#include "game.h"

#ifdef GAROU_HAVE_RECOMP
extern const ng_func_entry_t ng_recomp_table[];
extern const size_t ng_recomp_count;
#endif

int main(int argc, char **argv) {
#ifdef GAROU_HAVE_RECOMP
    return ng_main(argc, argv, &k_garou, ng_recomp_table, ng_recomp_count);
#else
    return ng_main(argc, argv, &k_garou, NULL, 0);
#endif
}
