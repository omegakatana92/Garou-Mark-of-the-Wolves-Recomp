#ifndef GAROU_ROM_SELECT_H
#define GAROU_ROM_SELECT_H

#include <stddef.h>

/*
 * If the command line has no --rom-path and a usable ROM folder cannot be
 * found automatically, show a native folder picker.  On success the chosen
 * (or cached, decrypted) directory is written to out and 1 is returned.
 * Returns 0 when the caller should leave the arguments alone (an explicit
 * --rom-path, --headless or --help), and -1 when the user cancelled or no
 * usable folder was selected.
 *
 * If the selected directory holds SNK's encrypted original set, the bundled
 * garou_decrypt helper is run into the per-user data directory and that
 * decrypted directory is returned.
 */
int garou_maybe_select_rom(int argc, char **argv, char *out, size_t out_size);

#endif
