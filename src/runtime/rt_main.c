/* The main of a console program (S5 Q8 (a)). langc build compiles this
   unit with the front end, the driver, the domain text and the program
   text. At the start it checks the program again, then runs main.
   Exit 0: main gives ret. Exit 1: a run time refusal, with the diagnostic
   on stderr. */
#define _POSIX_C_SOURCE 200809L
#include <signal.h>
#include <stdio.h>

#include "front/check.h"
#include "front/front.h"
#include "runtime/driver.h"

#define ARENA_LIMIT_BYTES ((size_t)1 << 30)

/* build.c gives the program text as one more unit. */
extern const unsigned char program_source[];
extern const size_t program_source_len;

int main(int argc, char **argv) {
  Diag diag;
  Arena arena;
  DeclList decls;
  Machine machine;
  uint32_t index = 0;
  int status = 1;
  signal(SIGPIPE, SIG_IGN);
  diag_init(&diag);
  arena_init(&arena, ARENA_LIMIT_BYTES);
  if (front_load(&arena, "main", (const char *)program_source, program_source_len, &decls, &diag)
      && check_program(&arena, &decls, &machine, &diag) && io_main(&machine, &index))
    status = io_run(&machine, def_value(&machine, index), EVAL_FUEL_STEPS, stdin, stdout, argc, argv);
  if (status != 0) diag_print(&diag, stderr);
  arena_release(&arena);
  return status;
}
