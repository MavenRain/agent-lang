/* JSON text for the S3 base span (S3 chunk B): jsonParse, jsonRender and
   the UTF-8 test of textUtf8 (see front/jsontext.h). SHARED by the tcc
   kits. Only the public front API is used. */
#include "front/jsontext.h"

#include <string.h>

#define JSON_DEPTH_MAX 512u

enum { J_NULL, J_BOOL, J_NUM, J_STR, J_ARR, J_OBJ, J_LNIL, J_LCONS, J_FNIL, J_FCONS, J_COUNT };

static const char *const CTOR_NAMES[J_COUNT] = {"jnull", "jbool", "jnum", "jstr", "jarr", "jobj", "jlNil", "jlCons", "jfNil", "jfCons"};
static const char *const CTOR_FAMILIES[J_COUNT] = {"Json", "Json", "Json", "Json", "Json", "Json", "JsonList", "JsonList", "JsonFields", "JsonFields"};
static const uint32_t CTOR_FIELDS[J_COUNT] = {0u, 1u, 1u, 1u, 1u, 1u, 0u, 2u, 0u, 3u};

/* The continuation byte count of the UTF-8 lead byte C, or 4 when C
   cannot start a sequence (0x80 to 0xc1, 0xf5 to 0xff). */
static unsigned utf8_need(unsigned c) {
  if (c < 0x80u)
    return 0u;
  if (c >= 0xc2u && c <= 0xdfu)
    return 1u;
  if (c >= 0xe0u && c <= 0xefu)
    return 2u;
  if (c >= 0xf0u && c <= 0xf4u)
    return 3u;
  return 4u;
}

int text_utf8_valid(const char *bytes, uint64_t len) {
  static const uint32_t MIN[4] = {0u, 0x80u, 0x800u, 0x10000u};
  uint64_t i = 0;
  while (i < len) {
    unsigned need = utf8_need((unsigned char)bytes[i]);
    uint32_t cp = (unsigned char)bytes[i] & (0x3fu >> need);
    unsigned k;
    if (need == 0u) {
      i++;
      continue;
    }
    if (need == 4u || need > len - i - 1u)
      return 0;
    for (k = 1u; k <= need; k++) {
      unsigned b = (unsigned char)bytes[i + k];
      if ((b & 0xc0u) != 0x80u)
        return 0;
      cp = (cp << 6) | (b & 0x3fu);
    }
    if (cp < MIN[need] || (cp >= 0xd800u && cp <= 0xdfffu) || cp > 0x10ffffu)
      return 0;
    i += need + 1u;
  }
  return 1;
}

/* The constructor indices of the Json block. */
typedef struct {
  uint32_t ctor[J_COUNT];
} Shapes;

enum { JT_BOOL, JT_NUMBER, JT_TEXT, JT_JSON, JT_LIST, JT_FIELDS, JT_COUNT };
static const unsigned CTOR_TYPES[J_COUNT][3] = {
  {0}, {JT_BOOL}, {JT_NUMBER}, {JT_TEXT}, {JT_LIST}, {JT_FIELDS},
  {0}, {JT_JSON, JT_LIST}, {0}, {JT_TEXT, JT_JSON, JT_FIELDS}
};

/* The evaluator constructs these fields without elaboration. Validate
   their types as well as the constructor and family arities. */
static int shapes_find(Machine *m, Shapes *s) {
  uint32_t k;
  uint32_t i;
  const Value *types[JT_COUNT];
  for (k = 0; k < J_COUNT; k++) {
    s->ctor[k] = m->ctor_count;
    for (i = 0; i < m->ctor_count; i++) {
      const CtorInfo *c = &m->ctors[i];
      if (strcmp(c->name, CTOR_NAMES[k]) == 0 && c->family < m->family_count && strcmp(m->families[c->family].name, CTOR_FAMILIES[k]) == 0 && c->field_count == CTOR_FIELDS[k] && c->interval_count == 0u && m->families[c->family].param_count == 0u && m->families[c->family].index_count == 0u && m->families[c->family].ctor_count == (k <= J_OBJ ? 6u : 2u))
        s->ctor[k] = i;
    }
    if (s->ctor[k] == m->ctor_count)
      return diag_fail(m->diag, "JSON_FAMILY", m->def, "the domain needs the standard Json constructor %s", CTOR_NAMES[k]);
  }
  types[JT_TEXT] = val_make(m, OP_TEXT, 0, 0, NULL, NULL, NULL);
  types[JT_BOOL] = val_make(m, OP_FIN, 0, 1, val_nat(m, 2u), NULL, NULL);
  types[JT_NUMBER] = val_make(m, OP_SUM, 0, 2, val_make(m, OP_INT64, 0, 0, NULL, NULL, NULL), types[JT_TEXT], NULL);
  types[JT_JSON] = val_op(m, VAL_OP, OP_FAMILY, m->ctors[s->ctor[J_NULL]].family, 0, NULL, 0);
  types[JT_LIST] = val_op(m, VAL_OP, OP_FAMILY, m->ctors[s->ctor[J_LNIL]].family, 0, NULL, 0);
  types[JT_FIELDS] = val_op(m, VAL_OP, OP_FAMILY, m->ctors[s->ctor[J_FNIL]].family, 0, NULL, 0);
  for (i = 0; i < JT_COUNT; i++) {
    if (types[i] == NULL)
      return 0;
  }
  for (k = 0; k < J_COUNT; k++) {
    const CtorInfo *c = &m->ctors[s->ctor[k]];
    const Env *env = NULL;
    for (i = 0; i < c->field_count; i++) {
      const Value *type = eval_core(m, env, c->fields[i].type);
      if (type == NULL)
        return 0;
      if (!conv_values(m, i, type, types[CTOR_TYPES[k][i]]))
        return diag_fail(m->diag, "JSON_FAMILY", m->def, "the domain has the wrong type for field %u of %s", i, CTOR_NAMES[k]);
      env = env_push(m, env, val_var(m, i));
      if (env == NULL)
        return 0;
    }
  }
  return 1;
}

int json_shapes_valid(Machine *m) {
  Shapes s;
  return shapes_find(m, &s);
}

static int oom_fail(Machine *m) {
  return diag_fail(m->diag, "OOM", m->def, "the arena limit is reached");
}

/* A byte buffer in the arena, always ended by a 0 byte. */
typedef struct {
  Machine *m;
  char *bytes;
  uint64_t len;
  uint64_t cap;
} Out;

static void out_init(Out *o, Machine *m) {
  o->m = m;
  o->bytes = NULL;
  o->len = 0;
  o->cap = 0;
}

static int out_put(Out *o, const char *bytes, uint64_t len) {
  if (o->len + len + 1u > o->cap) {
    uint64_t cap = o->cap == 0u ? 64u : o->cap;
    char *grown;
    while (cap < o->len + len + 1u)
      cap *= 2u;
    grown = cap > (uint64_t)SIZE_MAX ? NULL : arena_alloc(o->m->arena, (size_t)cap);
    if (grown == NULL)
      return oom_fail(o->m);
    if (o->len > 0u)
      memcpy(grown, o->bytes, (size_t)o->len);
    o->bytes = grown;
    o->cap = cap;
  }
  if (len > 0u)
    memcpy(o->bytes + o->len, bytes, (size_t)len);
  o->len += len;
  o->bytes[o->len] = '\0';
  return 1;
}

static const Value *out_text(Out *o) {
  return val_text(o->m, o->bytes == NULL ? "" : o->bytes, o->len);
}

/* The decimal digits of N, written at the end of BUF (21 bytes). */
static const char *u64_text(uint64_t n, char *buf) {
  char *at = buf + 20;
  *at = '\0';
  do {
    *--at = (char)('0' + (int)(n % 10u));
    n /= 10u;
  } while (n != 0u);
  return at;
}

/* ---- jsonRender ---- */

typedef struct {
  Machine *m;
  const Shapes *s;
  const Value *root;
  Out out;
  const Value *halt; /* the trap or the stuck jsonRender; NULL on a failure */
} Render;

/* 1 when V is a value of the kind KIND. Else 0 with HALT the trap, the
   stuck jsonRender, or NULL and an INTERNAL diagnostic. */
static int render_value(Render *r, const Value *v, ValKind kind) {
  if (v->kind == kind)
    return 1;
  if (v->kind == VAL_TRAP) {
    r->halt = v;
    return 0;
  }
  if (v->kind == VAL_VAR || v->kind == VAL_APP || v->kind == VAL_STUCK || v->kind == VAL_PAPP || val_is(v, OP_GLUE_ELEM)) {
    r->halt = val_op(r->m, VAL_STUCK, OP_JSON_RENDER, 0, 0, &r->root, 1);
    return 0;
  }
  return diag_fail(r->m->diag, "INTERNAL", r->m->def, "the evaluator found a Json value of the wrong shape");
}

static int is_ctor(const Render *r, const Value *v, unsigned k) {
  return v->op == OP_CTOR && v->inst == r->s->ctor[k] && v->argc >= CTOR_FIELDS[k];
}

static int wrong(Render *r) {
  return diag_fail(r->m->diag, "INTERNAL", r->m->def, "the evaluator found a Json value of the wrong shape");
}

static int put(Render *r, const char *text) {
  return out_put(&r->out, text, strlen(text));
}

/* The JSON form of the byte C in a string, into ESC; returns its length:
   \" and \\, \uXXXX for a control byte, else the byte itself. */
static uint64_t escape_byte(unsigned c, char *esc) {
  static const char HEX[] = "0123456789abcdef";
  esc[0] = '\\';
  esc[1] = (char)c;
  if (c == '"' || c == '\\')
    return 2u;
  if (c >= 0x20u) {
    esc[0] = (char)c;
    return 1u;
  }
  esc[1] = 'u';
  esc[2] = '0';
  esc[3] = '0';
  esc[4] = HEX[c >> 4];
  esc[5] = HEX[c & 15u];
  return 6u;
}

/* A string: the bytes go out as they are, also when they are not valid
   UTF-8. */
static int render_text(Render *r, const Value *t) {
  uint64_t i;
  char esc[6];
  if (!render_value(r, t, VAL_TEXT) || !put(r, "\""))
    return 0;
  for (i = 0; i < t->nat; i++) {
    if (!out_put(&r->out, esc, escape_byte((unsigned char)t->name[i], esc)))
      return 0;
  }
  return put(r, "\"");
}

/* jnum (inl n): n in decimal. jnum (inr t): t as it is. */
static int render_num(Render *r, const Value *s) {
  const Value *x;
  char buf[21];
  if (!render_value(r, s, VAL_OP))
    return 0;
  if (s->argc == 0u || (!val_is(s, OP_INL) && !val_is(s, OP_INR)))
    return wrong(r);
  x = s->args[s->argc - 1u];
  if (val_is(s, OP_INR))
    return render_value(r, x, VAL_TEXT) && out_put(&r->out, x->name, x->nat);
  if (!render_value(r, x, VAL_INT64))
    return 0;
  if (x->i64 < 0)
    return put(r, "-") && put(r, u64_text(0u - (uint64_t)x->i64, buf));
  return put(r, u64_text((uint64_t)x->i64, buf));
}

static int render_json(Render *r, const Value *v, uint32_t depth);

/* The items of a JsonList, or the members of a JsonFields (FIELDS 1). */
static int render_items(Render *r, const Value *v, uint32_t depth, int fields) {
  unsigned nil = fields ? J_FNIL : J_LNIL;
  unsigned cons = fields ? J_FCONS : J_LCONS;
  int first = 1;
  if (!put(r, fields ? "{" : "["))
    return 0;
  for (;;) {
    if (!render_value(r, v, VAL_OP))
      return 0;
    if (is_ctor(r, v, nil))
      return put(r, fields ? "}" : "]");
    if (!is_ctor(r, v, cons))
      return wrong(r);
    if (!first && !put(r, ","))
      return 0;
    if (fields && (!render_text(r, v->args[0]) || !put(r, ":")))
      return 0;
    if (!render_json(r, v->args[fields ? 1u : 0u], depth + 1u))
      return 0;
    first = 0;
    v = v->args[fields ? 2u : 1u];
  }
}

static int render_json(Render *r, const Value *v, uint32_t depth) {
  if (depth > JSON_DEPTH_MAX)
    return diag_fail(r->m->diag, "EVAL_DEPTH", r->m->def, "the Json value nests deeper than %u levels", JSON_DEPTH_MAX);
  if (!render_value(r, v, VAL_OP))
    return 0;
  if (is_ctor(r, v, J_NULL))
    return put(r, "null");
  if (is_ctor(r, v, J_BOOL))
    return render_value(r, v->args[0], VAL_NAT) && put(r, v->args[0]->nat == 1u ? "true" : "false");
  if (is_ctor(r, v, J_NUM))
    return render_num(r, v->args[0]);
  if (is_ctor(r, v, J_STR))
    return render_text(r, v->args[0]);
  if (is_ctor(r, v, J_ARR))
    return render_items(r, v->args[0], depth, 0);
  if (is_ctor(r, v, J_OBJ))
    return render_items(r, v->args[0], depth, 1);
  return wrong(r);
}

const Value *json_render_value(Machine *m, const Value *json) {
  Shapes s;
  Render r;
  if (!shapes_find(m, &s))
    return NULL;
  r.m = m;
  r.s = &s;
  r.root = json;
  r.halt = NULL;
  out_init(&r.out, m);
  if (!render_json(&r, json, 0u))
    return r.halt;
  return out_text(&r.out);
}

/* ---- jsonParse ---- */

typedef struct {
  Machine *m;
  const Shapes *s;
  const char *bytes;
  uint64_t len;
  uint64_t pos;
  const char *error; /* the parse error, or NULL */
  uint64_t error_pos;
} Parse;

/* A growing array of values in the arena. */
typedef struct {
  const Value **items;
  uint32_t count;
  uint32_t cap;
} Items;

static int items_push(Machine *m, Items *b, const Value *v) {
  if (v == NULL)
    return 0;
  if (b->count == b->cap) {
    uint32_t cap = b->cap == 0u ? 16u : b->cap * 2u;
    const Value **grown = cap < b->cap ? NULL : arena_alloc(m->arena, (size_t)cap * sizeof *grown);
    if (grown == NULL)
      return oom_fail(m);
    if (b->count > 0u)
      memcpy(grown, b->items, (size_t)b->count * sizeof *grown);
    b->items = grown;
    b->cap = cap;
  }
  b->items[b->count++] = v;
  return 1;
}

/* Records the parse error WHAT at the position; returns 0. */
static int parse_fail(Parse *p, const char *what) {
  p->error = what;
  p->error_pos = p->pos;
  return 0;
}

static const Value *parse_error(Parse *p, const char *what) {
  parse_fail(p, what);
  return NULL;
}

/* The byte at the position, or -1 at the end. */
static int peek(const Parse *p) {
  return p->pos < p->len ? (unsigned char)p->bytes[p->pos] : -1;
}

static void skip_ws(Parse *p) {
  while (peek(p) == ' ' || peek(p) == '\t' || peek(p) == '\n' || peek(p) == '\r')
    p->pos++;
}

static int is_digit(int c) {
  return c >= '0' && c <= '9';
}

static const Value *ctor(Parse *p, unsigned k, const Value *a, const Value *b, const Value *c) {
  const Value *f[3];
  f[0] = a;
  f[1] = b;
  f[2] = c;
  return val_ctor(p->m, p->s->ctor[k], f, CTOR_FIELDS[k]);
}

static const Value *parse_value(Parse *p, uint32_t depth);

/* The 4 hex digits after \u. -1 when one is not a hex digit. */
static long hex4(Parse *p) {
  long u = 0;
  unsigned k;
  for (k = 0; k < 4u; k++) {
    int c = peek(p);
    int d = is_digit(c) ? c - '0' : (c >= 'a' && c <= 'f') ? c - 'a' + 10 : (c >= 'A' && c <= 'F') ? c - 'A' + 10 : -1;
    if (d < 0)
      return -1;
    u = u * 16 + d;
    p->pos++;
  }
  return u;
}

/* The UTF-8 bytes of the code point CP. */
static int put_code(Out *o, uint32_t cp) {
  char b[4];
  if (cp < 0x80u) {
    b[0] = (char)cp;
    return out_put(o, b, 1u);
  }
  if (cp < 0x800u) {
    b[0] = (char)(0xc0u | (cp >> 6));
    b[1] = (char)(0x80u | (cp & 0x3fu));
    return out_put(o, b, 2u);
  }
  if (cp < 0x10000u) {
    b[0] = (char)(0xe0u | (cp >> 12));
    b[1] = (char)(0x80u | ((cp >> 6) & 0x3fu));
    b[2] = (char)(0x80u | (cp & 0x3fu));
    return out_put(o, b, 3u);
  }
  b[0] = (char)(0xf0u | (cp >> 18));
  b[1] = (char)(0x80u | ((cp >> 12) & 0x3fu));
  b[2] = (char)(0x80u | ((cp >> 6) & 0x3fu));
  b[3] = (char)(0x80u | (cp & 0x3fu));
  return out_put(o, b, 4u);
}

/* One escape at the backslash. A \u surrogate must be a high one followed
   by a \u low one. 0 with the error set, or 0 on a failure. */
static int parse_escape(Parse *p, Out *o) {
  static const char FROM[] = "\"\\/bfnrt";
  static const char TO[] = "\"\\/\b\f\n\r\t";
  const char *at;
  long u;
  long low;
  uint64_t start = p->pos;
  int e;
  p->pos++;
  e = peek(p);
  at = e > 0 ? strchr(FROM, e) : NULL;
  p->pos++;
  if (at != NULL)
    return out_put(o, TO + (at - FROM), 1u);
  u = e == 'u' ? hex4(p) : -1;
  if (u >= 0xd800 && u <= 0xdbff && peek(p) == '\\' && p->pos + 1u < p->len && p->bytes[p->pos + 1u] == 'u') {
    p->pos += 2u;
    low = hex4(p);
    u = low >= 0xdc00 && low <= 0xdfff ? 0x10000 + ((u - 0xd800) << 10) + (low - 0xdc00) : -1;
  }
  if (u < 0 || (u >= 0xd800 && u <= 0xdfff)) {
    p->pos = start;
    return parse_fail(p, "an escape is not valid");
  }
  return put_code(o, (uint32_t)u);
}

/* A string at its opening quote, as a Text. Bytes at or above 0x80 are
   kept as they are. */
static const Value *parse_text(Parse *p) {
  Out o;
  out_init(&o, p->m);
  p->pos++;
  for (;;) {
    int c = peek(p);
    if (c < 0)
      return parse_error(p, "a string has no end");
    if (c == '"') {
      p->pos++;
      return out_text(&o);
    }
    if (c < 0x20)
      return parse_error(p, "a control byte is in a string");
    if (c != '\\') {
      if (!out_put(&o, p->bytes + p->pos, 1u))
        return NULL;
      p->pos++;
      continue;
    }
    if (!parse_escape(p, &o))
      return NULL;
  }
}

/* A number: jnum (inl n) for an integer that fits Int64, else jnum (inr
   the number text) (delta (b)). */
static const Value *parse_number(Parse *p) {
  uint64_t start = p->pos;
  uint64_t mag = 0;
  int neg = peek(p) == '-';
  int whole = 1;
  int wide = 0;
  const Value *x;
  if (neg)
    p->pos++;
  if (!is_digit(peek(p)))
    return parse_error(p, "a number has no digit");
  if (peek(p) == '0') {
    p->pos++;
    if (is_digit(peek(p)))
      return parse_error(p, "a number has a leading zero");
  }
  while (is_digit(peek(p))) {
    unsigned d = (unsigned)(peek(p) - '0');
    wide = wide || mag > (UINT64_MAX - d) / 10u;
    mag = mag * 10u + d;
    p->pos++;
  }
  if (peek(p) == '.') {
    whole = 0;
    p->pos++;
    if (!is_digit(peek(p)))
      return parse_error(p, "a fraction has no digit");
    while (is_digit(peek(p)))
      p->pos++;
  }
  if (peek(p) == 'e' || peek(p) == 'E') {
    whole = 0;
    p->pos++;
    if (peek(p) == '+' || peek(p) == '-')
      p->pos++;
    if (!is_digit(peek(p)))
      return parse_error(p, "an exponent has no digit");
    while (is_digit(peek(p)))
      p->pos++;
  }
  if (whole && !wide && mag <= (neg ? (uint64_t)INT64_MAX + 1u : (uint64_t)INT64_MAX))
    x = val_make(p->m, OP_INL, 0, 1, val_int64(p->m, neg ? (int64_t)(0u - mag) : (int64_t)mag), NULL, NULL);
  else
    x = val_make(p->m, OP_INR, 0, 1, val_text(p->m, p->bytes + start, p->pos - start), NULL, NULL);
  return ctor(p, J_NUM, x, NULL, NULL);
}

static const Value *parse_word(Parse *p, const char *word, const Value *v) {
  size_t n = strlen(word);
  if (p->len - p->pos < n || memcmp(p->bytes + p->pos, word, n) != 0)
    return parse_error(p, "expected a value");
  p->pos += n;
  return v;
}

/* The members after an opening bracket, up to and with the closing
   bracket CLOSE: values, or key, colon and value for an object (FIELDS
   1). Duplicate keys stay, in their order. */
static int parse_members(Parse *p, Items *b, uint32_t depth, int fields, int close) {
  for (;;) {
    skip_ws(p);
    if (fields && peek(p) != '"')
      return parse_fail(p, "expected a string key");
    if (fields && !items_push(p->m, b, parse_text(p)))
      return 0;
    skip_ws(p);
    if (fields && peek(p) != ':')
      return parse_fail(p, "expected a colon");
    p->pos += fields ? 1u : 0u;
    if (!items_push(p->m, b, parse_value(p, depth + 1u)))
      return 0;
    skip_ws(p);
    if (peek(p) == close) {
      p->pos++;
      return 1;
    }
    if (peek(p) != ',')
      return parse_fail(p, fields ? "expected a comma or }" : "expected a comma or ]");
    p->pos++;
  }
}

/* An array (FIELDS 0) or an object (FIELDS 1) at its opening bracket. */
static const Value *parse_items(Parse *p, uint32_t depth, int fields) {
  Items b;
  const Value *acc;
  uint32_t i;
  int close = fields ? '}' : ']';
  b.items = NULL;
  b.count = 0;
  b.cap = 0;
  p->pos++;
  skip_ws(p);
  if (peek(p) == close)
    p->pos++;
  else if (!parse_members(p, &b, depth, fields, close))
    return NULL;
  acc = ctor(p, fields ? J_FNIL : J_LNIL, NULL, NULL, NULL);
  for (i = b.count; i > 0u && acc != NULL; i -= fields ? 2u : 1u)
    acc = fields ? ctor(p, J_FCONS, b.items[i - 2u], b.items[i - 1u], acc) : ctor(p, J_LCONS, b.items[i - 1u], acc, NULL);
  return acc == NULL ? NULL : ctor(p, fields ? J_OBJ : J_ARR, acc, NULL, NULL);
}

static const Value *parse_value(Parse *p, uint32_t depth) {
  skip_ws(p);
  if (depth > JSON_DEPTH_MAX)
    return parse_error(p, "the nesting is deeper than 512 levels");
  switch (peek(p)) {
  case '{':
    return parse_items(p, depth, 1);
  case '[':
    return parse_items(p, depth, 0);
  case '"':
    return ctor(p, J_STR, parse_text(p), NULL, NULL);
  case 'n':
    return parse_word(p, "null", ctor(p, J_NULL, NULL, NULL, NULL));
  case 't':
    return parse_word(p, "true", ctor(p, J_BOOL, val_nat(p->m, 1u), NULL, NULL));
  case 'f':
    return parse_word(p, "false", ctor(p, J_BOOL, val_nat(p->m, 0u), NULL, NULL));
  default:
    if (peek(p) == '-' || is_digit(peek(p)))
      return parse_number(p);
    return parse_error(p, "expected a value");
  }
}

const Value *json_parse_value(Machine *m, const Value *text) {
  Shapes s;
  Parse p;
  Out o;
  char buf[21];
  const char *digits;
  const Value *v;
  if (!shapes_find(m, &s))
    return NULL;
  p.m = m;
  p.s = &s;
  p.bytes = text->name;
  p.len = text->nat;
  p.pos = 0;
  p.error = NULL;
  p.error_pos = 0;
  v = parse_value(&p, 0u);
  skip_ws(&p);
  if (v != NULL && p.pos < p.len)
    v = parse_error(&p, "expected the end of the text");
  if (v != NULL)
    return val_make(m, OP_INR, 0, 1, v, NULL, NULL);
  if (p.error == NULL)
    return NULL;
  digits = u64_text(p.error_pos, buf);
  out_init(&o, m);
  if (!out_put(&o, "byte ", 5u) || !out_put(&o, digits, strlen(digits)) || !out_put(&o, ": ", 2u) || !out_put(&o, p.error, strlen(p.error)))
    return NULL;
  return val_make(m, OP_INL, 0, 1, out_text(&o), NULL, NULL);
}
