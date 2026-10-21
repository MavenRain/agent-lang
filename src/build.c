/* langc build (S5 Q7 and Q8 (a)). libtcc compiles each runtime unit of
   build/runtime.c and one unit with the program text in memory, then
   writes the executable. No C file goes to the disk. */
#include <stdio.h>
#include <string.h>

#include <libtcc.h>

#include "build.h"
#include "runtime/driver.h"

#ifndef TCC_LIB_DIR
#error "the Makefile gives TCC_LIB_DIR, the directory of libtcc1.a and include/"
#endif

/* build/runtime.c (gen/embed.c): the tcc -E text of each runtime unit. */
extern const char *const runtime_units[];
extern const size_t runtime_unit_count;

int build_main(Machine *m) {
  uint32_t index = 0;
  uint32_t io = 0;
  const Value *unit = NULL;
  const Value *type = NULL;
  if (!io_main(m, &index)) return 0;
  unit = io_family(m, &io) ? val_make(m, OP_UNIT, 0, 0, NULL, NULL, NULL) : NULL;
  type = unit == NULL ? NULL : val_op(m, VAL_OP, OP_FAMILY, io, 0, &unit, 1u);
  return type != NULL && conv_values(m, 0u, m->defs[index].type, type)
    ? 1 : diag_fail(m->diag, "BUILD_MAIN_TYPE", "main", "main does not have the type IO Unit");
}

static void tcc_error(void *opaque, const char *msg) {
  Diag *diag = opaque;
  if (!diag->set) diag_fail(diag, "BUILD_TCC", NULL, "%s", msg);
}

/* The program unit: the bytes of TEXT as program_source. */
static const char *program_unit(Arena *arena, const char *text, size_t len) {
  static const char head[] = "#include <stddef.h>\nconst unsigned char program_source[] = {";
  size_t cap = sizeof head + len * 5u + 64u;
  char *buf = arena_alloc(arena, cap);
  size_t at = sizeof head - 1u;
  if (buf == NULL) return NULL;
  memcpy(buf, head, at);
  for (size_t i = 0; i < len; i++)
    at += (size_t)snprintf(buf + at, cap - at, "%u,", (unsigned)(unsigned char)text[i]);
  snprintf(buf + at, cap - at, "0};\nconst size_t program_source_len = %zu;\n", len);
  return buf;
}

int console_build(Arena *arena, const char *text, size_t len, const char *out_path, Diag *diag) {
  const char *unit = program_unit(arena, text, len);
  TCCState *s = unit == NULL ? NULL : tcc_new();
  int ok = s != NULL;
  if (unit == NULL) return diag_fail(diag, "OOM", NULL, "no memory for the program unit");
  if (s == NULL) return diag_fail(diag, "BUILD_TCC", NULL, "tcc_new failed");
  tcc_set_error_func(s, diag, tcc_error);
  tcc_set_lib_path(s, TCC_LIB_DIR);
  ok = tcc_set_options(s, "-w") == 0 && tcc_set_output_type(s, TCC_OUTPUT_EXE) == 0;
  for (size_t i = 0; ok && i < runtime_unit_count; i++)
    ok = tcc_compile_string(s, runtime_units[i]) == 0;
  ok = ok && tcc_compile_string(s, unit) == 0 && tcc_output_file(s, out_path) == 0;
  tcc_delete(s);
  return ok ? 1 : diag->set ? 0 : diag_fail(diag, "BUILD_TCC", NULL, "libtcc failed");
}
