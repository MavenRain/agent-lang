/* The main of a console program (S5 Q8 (a)). langc build compiles this
   unit with the front end, the driver, the domain text and the program
   text. At the start it checks the program again, then runs main.
   Exit 0: main gives ret. Exit 1: a run time refusal, with the diagnostic
   on stderr. */
#define _POSIX_C_SOURCE 200809L
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

#include "front/check.h"
#include "front/front.h"
#include "runtime/driver.h"

#define ARENA_LIMIT_BYTES ((size_t)1 << 30)

/* build.c gives the program text as one more unit. */
extern const unsigned char program_source[];
extern const size_t program_source_len;

/* The trash root (S6 Q6 and Q7 (a)): HOME/.Trash on macOS; on another
   system XDG_DATA_HOME/Trash, else HOME/.local/share/Trash. NULL with no
   HOME or a path longer than SIZE. */
static const char *trash_root(char *buf, size_t size) {
  const char *home = getenv("HOME");
  int len = -1;
#ifdef __APPLE__
  len = home == NULL ? -1 : snprintf(buf, size, "%s/.Trash", home);
#else
  const char *data = getenv("XDG_DATA_HOME");
  len = data != NULL && data[0] != '\0' ? snprintf(buf, size, "%s/Trash", data)
      : home == NULL ? -1 : snprintf(buf, size, "%s/.local/share/Trash", home);
#endif
  return len < 0 || (size_t)len >= size ? NULL : buf;
}

int main(int argc, char **argv) {
  Diag diag;
  Arena arena;
  DeclList decls;
  Machine machine;
  uint32_t index = 0;
  int status = 1;
  static char root[4096];
#ifdef __APPLE__
  IoEnv env = {EVAL_FUEL_STEPS, NULL, NULL, 0, NULL, NULL, TRASH_MACOS, 0};
#else
  IoEnv env = {EVAL_FUEL_STEPS, NULL, NULL, 0, NULL, NULL, TRASH_XDG, 0};
#endif
  env.in = stdin;
  env.out = stdout;
  env.argc = argc;
  env.argv = argv;
  env.trash_root = trash_root(root, sizeof root);
  signal(SIGPIPE, SIG_IGN);
  diag_init(&diag);
  arena_init(&arena, ARENA_LIMIT_BYTES);
  if (front_load(&arena, "main", (const char *)program_source, program_source_len, &decls, &diag)
      && check_program(&arena, &decls, &machine, &diag) && io_main(&machine, &index))
    status = io_run(&machine, def_value(&machine, index), &env);
  if (status != 0) diag_print(&diag, stderr);
  arena_release(&arena);
  return status;
}
