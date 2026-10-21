/* langc build (S5 Q7 and Q8 (a)): libtcc compiles the runtime units and
   the program text into an executable. */
#ifndef LANG_BUILD_H
#define LANG_BUILD_H
#include "front/core.h"

/* Tests that the program has a definition main of the type IO Unit.
   Returns 1, or 0 after a BUILD_MAIN or BUILD_MAIN_TYPE diagnostic. */
int build_main(Machine *m);
/* Writes the executable OUT_PATH of the program text TEXT. Returns 1, or 0
   after a BUILD_TCC diagnostic with the libtcc text. */
int console_build(Arena *arena, const char *text, size_t len, const char *out_path, Diag *diag);
#endif
