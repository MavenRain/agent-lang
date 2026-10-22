/* The IO driver of a console program (S5 Q6 (a)). */
#ifndef LANG_RUNTIME_DRIVER_H
#define LANG_RUNTIME_DRIVER_H
#include <stdio.h>

#include "front/core.h"

/* The trash form (S6 Q6 (a)): HOME/.Trash on macOS, the XDG trash
   (files/ and info/) on another system. */
typedef enum { TRASH_MACOS, TRASH_XDG } TrashForm;

/* The environment of a run (S6 Q7 (a)). The fuel is BUDGET at the start
   and again after each terminal operation. readLine reads IN, write
   writes OUT, and args gives the arguments after ARGV[0]. TRASH_ROOT is
   the trash directory (NULL: no HOME), in the form TRASH_FORM. FORCE_EXDEV
   is the test field of Q4: when it is not 0, move takes each rename as
   EXDEV. The console program gives 0. OLLAMA_HOST is the OLLAMA_HOST
   value (NULL: the default endpoint) and OLLAMA_TIMEOUT_MS the read and
   write timeout of ollama (0: no timeout; S7 Q9 and Q15 (a)). */
typedef struct {
  uint64_t budget;
  FILE *in;
  FILE *out;
  int argc;
  char *const *argv;
  const char *trash_root;
  TrashForm trash_form;
  int force_exdev;
  const char *ollama_host;
  uint32_t ollama_timeout_ms;
} IoEnv;

/* The Ollama endpoint (S7 Q4 (a)): the host (no brackets) and the port.
   V6 is 1 for the form [V6]. */
typedef struct {
  char host[256];
  unsigned port;
  int v6;
} OllamaEndpoint;

/* Parses the OLLAMA_HOST value TEXT into *OUT: HOST, HOST:PORT, [V6] or
   [V6]:PORT, each with an optional http://. NULL or an empty TEXT gives
   127.0.0.1:11434. Returns NULL, or the error text. */
const char *ollama_endpoint(const char *text, OllamaEndpoint *out);

/* Finds the cofamily IO of one parameter. Returns 1, or 0. */
int io_family(const Machine *m, uint32_t *out);
/* Finds the definition main. Returns 1, or 0 after a BUILD_MAIN
   diagnostic. */
int io_main(Machine *m, uint32_t *index);
/* Runs the IO value V in ENV. Each out of V spends 1 fuel. Returns 0 at
   ret, or 1 after a diagnostic. */
int io_run(Machine *m, const Value *v, const IoEnv *env);
#endif
