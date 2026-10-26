/*
 * main.c - the Garou: Mark of the Wolves player.
 *
 * The hardware model, the interpreter fallback and the window all live in
 * neogeorecomp.  This file hands the runtime the ROM layout and the table of
 * routines that garou_recomp generated from the user's own dump at build
 * time.  Without that table the runtime interprets.
 *
 * On a first launch with no --rom-path and no discoverable ROM folder, a
 * native folder picker is shown.  If the user points at the original
 * encrypted set, the bundled garou_decrypt helper prepares it first.
 */
#include "game.h"
#include "rom_select.h"
#include <stdlib.h>
#include <string.h>

#ifdef GAROU_HAVE_RECOMP
extern const ng_func_entry_t ng_recomp_table[];
extern const size_t ng_recomp_count;
#endif

int main(int argc, char **argv) {
    char rom_path[4096];
    int selected = garou_maybe_select_rom(argc, argv, rom_path, sizeof(rom_path));
    if (selected < 0)
        return 0;   /* the user cancelled the picker */

    /* Drop --select-rom (it is a launcher option, not a runtime option) and
     * add the selected --rom-path. */
    char **args = (char **)malloc((size_t)(argc + 3) * sizeof(*args));
    if (!args) return 1;
    int n = 0;
    for (int i = 0; i < argc; i++) {
        if (!strcmp(argv[i], "--select-rom")) continue;
        args[n++] = argv[i];
    }
    if (selected > 0) {
        args[n++] = "--rom-path";
        args[n++] = rom_path;
    }
    args[n] = NULL;

    int rc;
#ifdef GAROU_HAVE_RECOMP
    rc = ng_main(n, args, &k_garou, ng_recomp_table, ng_recomp_count);
#else
    rc = ng_main(n, args, &k_garou, NULL, 0);
#endif
    free(args);
    return rc;
}
