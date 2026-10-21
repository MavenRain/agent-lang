/* The IO driver of a console program (S5 Q6 (a)). */
#ifndef LANG_RUNTIME_DRIVER_H
#define LANG_RUNTIME_DRIVER_H
#include <stdio.h>

#include "front/core.h"

/* Finds the cofamily IO of one parameter. Returns 1, or 0. */
int io_family(const Machine *m, uint32_t *out);
/* Finds the definition main. Returns 1, or 0 after a BUILD_MAIN
   diagnostic. */
int io_main(Machine *m, uint32_t *index);
/* Runs the IO value V. Each out of V spends 1 fuel. The fuel is BUDGET at
   the start and again after each terminal operation. readLine reads IN,
   write writes OUT, and args gives the arguments after ARGV[0]. Returns 0
   at ret, or 1 after a diagnostic. */
int io_run(Machine *m, const Value *v, uint64_t budget, FILE *in, FILE *out, int argc, char *const *argv);
#endif
