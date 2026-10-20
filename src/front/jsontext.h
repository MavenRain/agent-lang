/* JSON text and UTF-8 for the S3 base span (S3 chunk B). The Json,
   JsonList and JsonFields families are found by name (PLAN section 5,
   Q7 (a)). */
#ifndef LANG_FRONT_JSONTEXT_H
#define LANG_FRONT_JSONTEXT_H

#include "front/core.h"

/* 1 when the LEN bytes at BYTES are valid UTF-8: no overlong form, no
   surrogate, no code point above U+10FFFF. */
int text_utf8_valid(const char *bytes, uint64_t len);

/* Validate the complete domain schema used by the JSON primitives. */
int json_shapes_valid(Machine *m);

/* jsonParse at the Text value TEXT: inr of the Json value, or inl of an
   error text that starts with the byte offset. NULL on a failure (the
   diagnostic is set). */
const Value *json_parse_value(Machine *m, const Value *text);

/* jsonRender at the Json value JSON: the Text with no spaces. A trap in
   JSON gives the trap, a neutral part gives the stuck jsonRender. NULL on
   a failure (the diagnostic is set). */
const Value *json_render_value(Machine *m, const Value *json);

#endif
