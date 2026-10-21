/* The IO driver (S5 Q6 (a)): v := main; then out v (1 fuel). ret ends the
   run, tau t gives v := t, and an operation runs, then v := k r. The
   driver finds the layer constructors by name once at the start. */
#define _POSIX_C_SOURCE 200809L
#include <stdlib.h>
#include <string.h>

#include "runtime/driver.h"

typedef struct {
  uint32_t io;
  uint32_t ret;
  uint32_t tau;
  uint32_t read_line;
  uint32_t write;
  uint32_t args;
  uint32_t jstr;
  uint32_t jarr;
  uint32_t jl_nil;
  uint32_t jl_cons;
} IoNames;

typedef struct {
  uint64_t budget;
  FILE *in;
  FILE *out;
  int argc;
  char *const *argv;
} IoRun;

typedef enum {
  LAYER_RET,
  LAYER_TAU,
  LAYER_READ_LINE,
  LAYER_WRITE,
  LAYER_ARGS,
  LAYER_OTHER
} Layer;

static int find_family(const Machine *m, const char *name, uint32_t *out) {
  for (uint32_t i = 0; i < m->family_count; i++) {
    if (strcmp(m->families[i].name, name) == 0) {
      *out = i;
      return 1;
    }
  }
  return 0;
}

/* Finds the constructor NAME of FAMILY with FIELDS point fields. */
static int find_ctor(const Machine *m, uint32_t family, const char *name, uint32_t fields, uint32_t *out) {
  for (uint32_t i = 0; i < m->ctor_count; i++) {
    if (m->ctors[i].family == family && m->ctors[i].field_count == fields && strcmp(m->ctors[i].name, name) == 0) {
      *out = i;
      return 1;
    }
  }
  return 0;
}

int io_family(const Machine *m, uint32_t *out) {
  return find_family(m, "IO", out) && m->families[*out].cofamily && m->families[*out].param_count == 1u;
}

int io_main(Machine *m, uint32_t *index) {
  for (uint32_t i = 0; i < m->def_count; i++) {
    if (strcmp(m->defs[i].name, "main") == 0) {
      *index = i;
      m->def = m->defs[i].name;
      return 1;
    }
  }
  return diag_fail(m->diag, "BUILD_MAIN", NULL, "the program has no definition main");
}

static int io_names(const Machine *m, IoNames *n) {
  uint32_t layer = 0;
  uint32_t json = 0;
  uint32_t list = 0;
  int ok = io_family(m, &n->io);
  layer = ok ? m->families[n->io].layer : 0u;
  ok = ok && find_ctor(m, layer, "ret", 1u, &n->ret) && find_ctor(m, layer, "tau", 1u, &n->tau)
    && find_ctor(m, layer, "readLineOp", 1u, &n->read_line) && find_ctor(m, layer, "writeOp", 2u, &n->write)
    && find_ctor(m, layer, "argsOp", 1u, &n->args)
    && find_family(m, "Json", &json) && find_family(m, "JsonList", &list)
    && find_ctor(m, json, "jstr", 1u, &n->jstr) && find_ctor(m, json, "jarr", 1u, &n->jarr)
    && find_ctor(m, list, "jlNil", 0u, &n->jl_nil) && find_ctor(m, list, "jlCons", 2u, &n->jl_cons);
  return ok ? 1 : diag_fail(m->diag, "INTERNAL", m->def, "the domain has no IO layer of the S5 shape");
}

static Layer layer_of(const IoNames *n, const Value *layer) {
  if (!val_is(layer, OP_CTOR)) return LAYER_OTHER;
  if (layer->inst == n->ret) return LAYER_RET;
  if (layer->inst == n->tau) return LAYER_TAU;
  if (layer->inst == n->read_line) return LAYER_READ_LINE;
  if (layer->inst == n->write) return LAYER_WRITE;
  if (layer->inst == n->args) return LAYER_ARGS;
  return LAYER_OTHER;
}

/* readLine: some of the next line without its \n, or none at the end of
   the input or after a read error. */
static const Value *read_line(Machine *m, FILE *in) {
  char *line = NULL;
  size_t cap = 0;
  ssize_t got = getline(&line, &cap, in);
  size_t len = got > 0 ? (size_t)got : 0u;
  size_t keep = len > 0u && line[len - 1u] == '\n' ? len - 1u : len;
  char *copy = got < 0 ? NULL : arena_alloc(m->arena, keep + 1u);
  const Value *text = NULL;
  if (copy != NULL) {
    memcpy(copy, line, keep);
    copy[keep] = '\0';
    text = val_text(m, copy, keep);
  }
  free(line);
  if (got < 0) return val_make(m, OP_NONE, 0, 0, NULL, NULL, NULL);
  if (text == NULL) {
    diag_fail(m->diag, "OOM", m->def, "no memory for a line of %zu bytes", keep);
    return NULL;
  }
  return val_make(m, OP_SOME, 0, 1, text, NULL, NULL);
}

/* args: jarr of jstr of each argument after the program name. */
static const Value *args_json(Machine *m, const IoNames *n, int argc, char *const *argv) {
  const Value *list = val_ctor(m, n->jl_nil, NULL, 0u);
  for (int i = argc - 1; i >= 1 && list != NULL; i--) {
    const Value *text = val_text(m, argv[i], strlen(argv[i]));
    const Value *str = text == NULL ? NULL : val_ctor(m, n->jstr, &text, 1u);
    const Value *cell[2] = {str, list};
    list = str == NULL ? NULL : val_ctor(m, n->jl_cons, cell, 2u);
  }
  return list == NULL ? NULL : val_ctor(m, n->jarr, &list, 1u);
}

/* write: all the bytes of the text, then fflush. */
static int write_text(Machine *m, const Value *text, FILE *out) {
  if (text->kind != VAL_TEXT) return diag_fail(m->diag, "INTERNAL", m->def, "write got a value that is not a text");
  size_t count = fwrite(text->name, 1, (size_t)text->nat, out);
  int flushed = fflush(out);
  return count == text->nat && flushed == 0 ? 1 : diag_fail(m->diag, "IO_WRITE", m->def, "cannot write %llu bytes", (unsigned long long)text->nat);
}

/* One layer of *V. Returns -1 to go on, 0 at ret, or 1 after a
   diagnostic. */
static int io_step(Machine *m, const IoNames *n, const IoRun *run, const Value **v) {
  const Value *layer = eval_out(m, n->io, *v);
  const Value *const *field = layer == NULL ? NULL : layer->args + m->families[n->io].param_count + 1u;
  const Value *response = NULL;
  if (layer == NULL) return 1;
  switch (layer_of(n, layer)) {
  case LAYER_RET:
    return 0;
  case LAYER_TAU:
    *v = field[0];
    return -1;
  case LAYER_READ_LINE:
    response = read_line(m, run->in);
    break;
  case LAYER_ARGS:
    response = args_json(m, n, run->argc, run->argv);
    break;
  case LAYER_WRITE:
    if (!write_text(m, field[0], run->out)) return 1;
    m->fuel = run->budget;
    *v = field[1];
    return -1;
  case LAYER_OTHER:
    diag_fail(m->diag, "INTERNAL", m->def, "the IO driver found a layer that is not an IO constructor");
    return 1;
  }
  if (response == NULL) return 1;
  m->fuel = run->budget;
  *v = eval_apply(m, field[0], response);
  return *v == NULL ? 1 : -1;
}

int io_run(Machine *m, const Value *v, uint64_t budget, FILE *in, FILE *out, int argc, char *const *argv) {
  IoNames n;
  IoRun run = {budget, in, out, argc, argv};
  int status = v == NULL || !io_names(m, &n) ? 1 : -1;
  m->fuel = budget;
  while (status < 0)
    status = io_step(m, &n, &run, &v);
  return status;
}
