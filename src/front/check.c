/* The type checker (see front/check.h). SHARED by the tcc kits.

   Checking is bidirectional. A term is checked against an expected type (the
   hint) or its type is inferred. Two types are equal when their normal forms
   are equal (front/eval.c). The checker writes core terms (front/core.h). */
#include "front/check.h"

#include <string.h>

#define CHECK_DEPTH_LIMIT 2000u
#define SPINE_MAX 64u
#define SHOW_MAX 512u
#define NAMES_MAX 64u
#define UNIVERSE_MAX 2u
#define PRINT_MAX 65536u
#define NO_FAMILY UINT32_MAX
#define NO_DEF UINT32_MAX

#define FAIL(c, code, ...) diag_fail((c)->m->diag, (code), (c)->m->def, __VA_ARGS__)

typedef struct Scope Scope;
struct Scope {
  const char *name; /* NULL for the binder of `A -> B` */
  const Value *type;
  uint32_t level;
  const Scope *next;
};

typedef struct {
  Machine *m;
  const DeclList *decls;
  size_t decl_index;
  const Scope *scope; /* the innermost binder first */
  const Env *env;     /* a variable for each binder */
  uint32_t level;     /* the number of binders */
  unsigned depth;
  uint32_t eta_count;
  uint32_t self;      /* the family whose fields are checked, or NO_FAMILY */
  uint32_t self_uses; /* the uses of that family in the current field */
  /* The levels of the earlier point fields of the current field, from
     FIELD_BASE up to FIELD_TOP (S4 item 13), and the uses of them. */
  uint32_t field_base;
  uint32_t field_top;
  int field_dep;
  int field_rec;
} Checker;

typedef enum {
  RULE_TYPE0,
  RULE_TYPE1,
  RULE_TYPE2,
  RULE_INTERVAL,
  RULE_IEND,
  RULE_PATHP,
  RULE_PATH,
  RULE_VALUE0,
  RULE_PAIR,
  RULE_INJ,
  RULE_EMPTY,
  RULE_SOME,
  RULE_CONS,
  RULE_PACK,
  RULE_REFL,
  RULE_PART,
  RULE_SIGMA_PART,
  RULE_EITHER,
  RULE_OPTION,
  RULE_PURE,
  RULE_MAP,
  RULE_BIND,
  RULE_FILTER,
  RULE_FOLD,
  RULE_UNFOLD,
  RULE_SYM,
  RULE_CONG,
  RULE_FUNEXT,
  RULE_TRANSP,
  RULE_HCOMP,
  RULE_NAT2,
  RULE_FIN,
  RULE_FIN_OP,
  RULE_FIN_CASE,
  RULE_DEC_EQ,
  RULE_GLUE,
  RULE_GLUE_ELEM,
  RULE_UNGLUE,
  RULE_FLAG_IF,
  RULE_ABSURD
} Rule;

typedef struct {
  const char *name;
  uint32_t arity; /* fold: the least number of arguments */
  Rule rule;
  Op op;
  Op result;     /* VALUE0, NAT2, FIN_OP: the result type. EMPTY, INJ: the type former. */
  uint32_t part; /* INJ, PART, SIGMA_PART: 0 or 1. NAT2, FIN_OP: N of a Fin N result (0: the argument n) */
} Builtin;

static const Builtin BUILTINS[] = {
  {"Nat", 0, RULE_TYPE0, OP_NAT, OP_NAT, 0},
  {"Flag", 0, RULE_TYPE0, OP_FLAG, OP_NAT, 0},
  {"Unit", 0, RULE_TYPE0, OP_UNIT, OP_NAT, 0},
  {"Empty", 0, RULE_TYPE0, OP_EMPTY, OP_NAT, 0},
  {"Prod", 2, RULE_TYPE2, OP_PROD, OP_NAT, 0},
  {"Sum", 2, RULE_TYPE2, OP_SUM, OP_NAT, 0},
  {"Option", 1, RULE_TYPE1, OP_OPTION, OP_NAT, 0},
  {"List", 1, RULE_TYPE1, OP_LIST, OP_NAT, 0},
  {"Fin", 1, RULE_FIN, OP_FIN, OP_NAT, 0},
  {"finAdd", 3, RULE_FIN_OP, OP_FIN_ADD, OP_FIN, 0},
  {"finNeg", 2, RULE_FIN_OP, OP_FIN_NEG, OP_FIN, 0},
  {"finMul", 3, RULE_FIN_OP, OP_FIN_MUL, OP_FIN, 0},
  {"finVal", 2, RULE_FIN_OP, OP_FIN_VAL, OP_NAT, 0},
  {"finEq", 3, RULE_FIN_OP, OP_FIN_EQ, OP_FIN, 2},
  {"decEqFin", 3, RULE_DEC_EQ, OP_DEC_EQ_FIN, OP_SUM, 0},
  {"decEqNat", 2, RULE_DEC_EQ, OP_DEC_EQ_NAT, OP_SUM, 0},
  {"finCase", 2, RULE_FIN_CASE, OP_FIN_CASE, OP_FIN, 0},
  {"I", 0, RULE_INTERVAL, OP_INTERVAL, OP_NAT, 0},
  {"i0", 0, RULE_IEND, OP_INTERVAL, OP_INTERVAL, 0},
  {"i1", 0, RULE_IEND, OP_INTERVAL, OP_INTERVAL, 1},
  {"PathP", 3, RULE_PATHP, OP_INTERVAL, OP_NAT, 0},
  {"Path", 3, RULE_PATH, OP_INTERVAL, OP_NAT, 0},
  {"unit", 0, RULE_VALUE0, OP_UNIT_VAL, OP_UNIT, 0},
  {"flagYes", 0, RULE_VALUE0, OP_FLAG_YES, OP_FLAG, 0},
  {"flagNo", 0, RULE_VALUE0, OP_FLAG_NO, OP_FLAG, 0},
  {"pair", 2, RULE_PAIR, OP_PAIR, OP_PROD, 0},
  {"inl", 1, RULE_INJ, OP_INL, OP_SUM, 0},
  {"inr", 1, RULE_INJ, OP_INR, OP_SUM, 1},
  {"none", 0, RULE_EMPTY, OP_NONE, OP_OPTION, 0},
  {"nil", 0, RULE_EMPTY, OP_NIL, OP_LIST, 0},
  {"some", 1, RULE_SOME, OP_SOME, OP_OPTION, 0},
  {"cons", 2, RULE_CONS, OP_CONS, OP_LIST, 0},
  {"pack", 2, RULE_PACK, OP_PACK, OP_NAT, 0},
  {"refl", 0, RULE_REFL, OP_INTERVAL, OP_NAT, 0},
  {"first", 1, RULE_PART, OP_FIRST, OP_PROD, 0},
  {"second", 1, RULE_PART, OP_SECOND, OP_PROD, 1},
  {"witness", 1, RULE_SIGMA_PART, OP_WITNESS, OP_NAT, 0},
  {"payload", 1, RULE_SIGMA_PART, OP_PAYLOAD, OP_NAT, 1},
  {"either", 3, RULE_EITHER, OP_EITHER, OP_SUM, 0},
  {"option", 3, RULE_OPTION, OP_OPTION_ELIM, OP_OPTION, 0},
  {"pure", 1, RULE_PURE, OP_PURE, OP_NAT, 0},
  {"map", 2, RULE_MAP, OP_MAP, OP_NAT, 0},
  {"bind", 2, RULE_BIND, OP_BIND, OP_NAT, 0},
  {"filter", 2, RULE_FILTER, OP_FILTER, OP_NAT, 0},
  {"fold", 2, RULE_FOLD, OP_FOLD_NAT, OP_NAT, 0},
  {"unfold", 3, RULE_UNFOLD, OP_UNFOLD, OP_LIST, 0},
  {"sym", 1, RULE_SYM, OP_INTERVAL, OP_NAT, 0},
  {"cong", 2, RULE_CONG, OP_INTERVAL, OP_NAT, 0},
  {"funExt", 1, RULE_FUNEXT, OP_INTERVAL, OP_NAT, 0},
  {"transp", 3, RULE_TRANSP, OP_TRANSP, OP_NAT, 0},
  {"hcomp", 4, RULE_HCOMP, OP_HCOMP, OP_NAT, 0},
  {"Glue", 3, RULE_GLUE, OP_GLUE, OP_NAT, 0},
  {"glue", 2, RULE_GLUE_ELEM, OP_GLUE_ELEM, OP_NAT, 0},
  {"unglue", 1, RULE_UNGLUE, OP_UNGLUE, OP_NAT, 0},
  {"natAdd", 2, RULE_NAT2, OP_NAT_ADD, OP_NAT, 0},
  {"natSub", 2, RULE_NAT2, OP_NAT_SUB, OP_NAT, 0},
  {"natMul", 2, RULE_NAT2, OP_NAT_MUL, OP_NAT, 0},
  {"succ", 1, RULE_NAT2, OP_SUCC, OP_NAT, 0},
  {"natEq", 2, RULE_NAT2, OP_NAT_EQ, OP_FLAG, 0},
  {"natLe", 2, RULE_NAT2, OP_NAT_LE, OP_FLAG, 0},
  {"flagIf", 3, RULE_FLAG_IF, OP_FLAG_IF, OP_FLAG, 0},
  {"absurd", 2, RULE_ABSURD, OP_ABSURD, OP_NAT, 0}
};

#define BUILTIN_COUNT ((uint32_t)(sizeof BUILTINS / sizeof BUILTINS[0]))

/* Names that no declaration can take (rule R4). */
static const char *const RESERVED[] = {"Bool", "Text"};

typedef enum {
  HEAD_TERM, /* a local, a definition or another term: plain application */
  HEAD_FAMILY,
  HEAD_CTOR,
  HEAD_PROJ,
  HEAD_BUILTIN
} HeadKind;

typedef struct {
  HeadKind kind;
  uint32_t index; /* FAMILY: the family. CTOR, PROJ: the constructor. */
  uint32_t field; /* PROJ: the field number */
  const Builtin *builtin;
} Head;

/* One application of a core name to the arguments of a spine. */
typedef struct {
  Checker *c;
  const Builtin *b;
  const Term *const *args;
  uint32_t argc;      /* the arguments in the spine */
  const Value *hint;  /* the expected type of the result, or NULL */
  const Value *outer; /* the expected type of the whole spine, or NULL */
  uint32_t used;      /* the arguments that the core name takes */
  const Value *type;  /* the type of the result */
} Call;

static const Core *nul(int failed) {
  (void)failed;
  return NULL;
}

static const Value *vnul(int failed) {
  (void)failed;
  return NULL;
}

static Core *new_core(Checker *c, CoreKind kind) {
  Core *k = arena_alloc(c->m->arena, sizeof *k);
  if (k == NULL) {
    FAIL(c, "OOM", "out of memory");
    return NULL;
  }
  k->kind = kind;
  return k;
}

static const Core *core_index(Checker *c, CoreKind kind, uint32_t index) {
  Core *k = new_core(c, kind);
  if (k != NULL)
    k->index = index;
  return k;
}

static const Core *core_leaf(Checker *c, CoreKind kind, uint64_t nat) {
  Core *k = new_core(c, kind);
  if (k != NULL)
    k->nat = nat;
  return k;
}

static const Core *core_bind(Checker *c, CoreKind kind, const char *name, const Core *a, const Core *b) {
  Core *k;
  if (b == NULL || (a == NULL && kind != CORE_LAM && kind != CORE_PLAM))
    return NULL;
  k = new_core(c, kind);
  if (k == NULL)
    return NULL;
  k->name = name;
  k->a = a;
  k->b = b;
  return k;
}

static const Core *core_app(Checker *c, const Core *f, const Core *x) {
  Core *k;
  if (f == NULL || x == NULL)
    return NULL;
  k = new_core(c, CORE_APP);
  if (k == NULL)
    return NULL;
  k->a = f;
  k->b = x;
  return k;
}

/* CORE_PATHP (the line B) and CORE_PAPP (the path A at the interval B), with
   the ends E0 and E1. */
static const Core *core_ends(Checker *c, CoreKind kind, const char *name, const Core *a, const Core *b, const Core *e0, const Core *e1) {
  const Core **ends;
  Core *k;
  if (b == NULL || e0 == NULL || e1 == NULL || (a == NULL && kind != CORE_PATHP))
    return NULL;
  ends = arena_alloc(c->m->arena, 2u * sizeof *ends);
  if (ends == NULL)
    return nul(FAIL(c, "OOM", "out of memory"));
  k = new_core(c, kind);
  if (k == NULL)
    return NULL;
  ends[0] = e0;
  ends[1] = e1;
  k->name = name;
  k->a = a;
  k->b = b;
  k->args = ends;
  k->argc = 2;
  return k;
}

/* An interval term: CORE_INEG (A), CORE_IMEET and CORE_IJOIN (A, B). */
static const Core *core_iop(Checker *c, CoreKind kind, const Core *a, const Core *b) {
  Core *k;
  if (a == NULL || (b == NULL && kind != CORE_INEG))
    return NULL;
  k = new_core(c, kind);
  if (k != NULL) {
    k->a = a;
    k->b = b;
  }
  return k;
}

static const Core *core_op(Checker *c, Op op, uint32_t inst, uint32_t field, const Core *const *args, uint32_t n) {
  const Core **copy = arena_alloc(c->m->arena, ((size_t)n + 1u) * sizeof *copy);
  Core *k;
  uint32_t i;
  if (copy == NULL)
    return nul(FAIL(c, "OOM", "out of memory"));
  for (i = 0; i < n; i++) {
    if (args[i] == NULL)
      return NULL;
    copy[i] = args[i];
  }
  k = new_core(c, CORE_OP);
  if (k == NULL)
    return NULL;
  k->op = op;
  k->inst = inst;
  k->field = field;
  k->args = copy;
  k->argc = n;
  return k;
}

static const Core *op3(Checker *c, Op op, uint32_t inst, uint32_t n, const Core *a, const Core *b, const Core *x) {
  const Core *args[3];
  args[0] = a;
  args[1] = b;
  args[2] = x;
  return core_op(c, op, inst, 0, args, n);
}

static const Value *tyop(Checker *c, Op op, uint32_t n, const Value *a, const Value *b, const Value *x) {
  return val_make(c->m, op, 0, n, a, b, x);
}

static const Value *nat_type(Checker *c) {
  return tyop(c, OP_NAT, 0, NULL, NULL, NULL);
}

/* Flag: the result type of a filter predicate. */
static const Value *flag_type(Checker *c) {
  return tyop(c, OP_FLAG, 0, NULL, NULL, NULL);
}

/* The sort I: the type of an interval term, never a type itself. */
static const Value *interval_type(Checker *c) {
  return tyop(c, OP_INTERVAL, 0, NULL, NULL, NULL);
}

/* The value of the core term K in the current context. */
static const Value *here(Checker *c, const Core *k) {
  return k == NULL ? NULL : eval_core(c->m, c->env, k);
}

/* V in the surface syntax, for a diagnostic. */
static const char *show(Checker *c, const Value *v) {
  const char *names[NAMES_MAX];
  const Scope *s;
  char *buf = arena_alloc(c->m->arena, SHOW_MAX);
  uint32_t count = c->level < NAMES_MAX ? c->level : NAMES_MAX;
  for (s = c->scope; s != NULL; s = s->next) {
    if (s->level < NAMES_MAX)
      names[s->level] = s->name != NULL ? s->name : "_";
  }
  if (buf == NULL || v == NULL)
    return "?";
  value_print(c->m, names, count, v, buf, SHOW_MAX);
  return buf;
}

static int push_value(Checker *c, const char *name, const Value *type, const Value *value) {
  Scope *s = arena_alloc(c->m->arena, sizeof *s);
  const Env *e = env_push(c->m, c->env, value);
  if (type == NULL || value == NULL || s == NULL || e == NULL)
    return c->m->diag->set ? 0 : FAIL(c, "OOM", "out of memory");
  s->name = name;
  s->type = type;
  s->level = c->level;
  s->next = c->scope;
  c->scope = s;
  c->env = e;
  c->level++;
  return 1;
}

static int push(Checker *c, const char *name, const Value *type) {
  return push_value(c, name, type, val_var(c->m, c->level));
}

/* An interval binder: its value is an interval variable, not a neutral. */
static int push_interval(Checker *c, const char *name) {
  return push_value(c, name, interval_type(c), val_ivar(c->m, c->level));
}

static void pop(Checker *c) {
  c->scope = c->scope->next;
  c->env = c->env->next;
  c->level--;
}

/* 1 when a term of type FOUND can stand where WANT is expected.
   Universes are not cumulative (former F14). */
static int expect(Checker *c, const Value *found, const Value *want) {
  if (found == NULL || want == NULL)
    return 0;
  if (conv_values(c->m, c->level, found, want))
    return 1;
  return FAIL(c, "TYPE_MISMATCH", "expected a term of type %s, found a term of type %s", show(c, want), show(c, found));
}

static const Scope *find_local(const Checker *c, const char *name) {
  const Scope *s;
  for (s = c->scope; s != NULL; s = s->next) {
    if (s->name != NULL && strcmp(s->name, name) == 0)
      return s;
  }
  return NULL;
}

static uint32_t find_def(const Machine *m, const char *name) {
  uint32_t i;
  for (i = 0; i < m->def_count; i++) {
    if (strcmp(m->defs[i].name, name) == 0)
      return i;
  }
  return NO_DEF;
}

/* 1 when ANN is the sort I: the name I, not taken by a local or a
   definition. Only a binder annotation can name I. */
static int is_interval(const Checker *c, const Term *ann) {
  return ann != NULL && ann->kind == TERM_VAR && strcmp(ann->name, "I") == 0 && find_local(c, ann->name) == NULL && find_def(c->m, ann->name) == NO_DEF;
}

/* An environment that binds VALS in order: the last one is index 0. */
static const Env *env_of(Checker *c, const Value *const *vals, uint32_t n) {
  const Env *e = NULL;
  uint32_t i;
  for (i = 0; i < n; i++)
    e = env_push(c->m, e, vals[i]);
  return e;
}

/* The value of K, a term under one interval binder, with R for the binder. */
static const Value *at(Checker *c, const Core *k, const Value *r) {
  const Env *e = env_push(c->m, c->env, r);
  return k == NULL || e == NULL ? NULL : eval_core(c->m, e, k);
}

static int end_fail(Checker *c, int uip, const char *side, const Value *found, const Value *want) {
  if (uip)
    return FAIL(c, "PATH_UIP", "the path at %s is %s, but the type needs %s; these ends are paths, and only a rule of uniqueness of identity proofs can make them equal (rule R9)", side, show(c, found), show(c, want));
  return FAIL(c, "PATH_END", "the path at %s is %s, but the type needs %s", side, show(c, found), show(c, want));
}

/* The end check of a path against the path type E: V0 and V1 are its
   values at i0 and i1. */
static int path_ends(Checker *c, const Value *e, const Value *v0, const Value *v1) {
  const Value *ends = closure_apply(c->m, e, val_iend(c->m, 0));
  int uip = ends != NULL && ends->kind == VAL_PATHP;
  if (v0 == NULL || v1 == NULL)
    return 0;
  if (!conv_values(c->m, c->level, v0, e->args[0]))
    return end_fail(c, uip, "i0", v0, e->args[0]);
  return conv_values(c->m, c->level, v1, e->args[1]) ? 1 : end_fail(c, uip, "i1", v1, e->args[1]);
}

static const Core *elab(Checker *c, const Term *t, const Value *hint, const Value **type);
static const Core *check_lam(Checker *c, const Term *t, const Value *hint, const Value **type);

static const Core *check(Checker *c, const Term *t, const Value *want) {
  const Value *type = NULL;
  return want == NULL ? NULL : elab(c, t, want, &type);
}

static const Core *infer(Checker *c, const Term *t, const Value **type) {
  *type = NULL;
  return elab(c, t, NULL, type);
}

/* infer, but a failure leaves no diagnostic. */
static const Core *try_infer(Checker *c, const Term *t, const Value **type) {
  Diag saved = *c->m->diag;
  const Core *k = infer(c, t, type);
  if (k == NULL)
    *c->m->diag = saved;
  return k;
}

static const Core *check_type(Checker *c, const Term *t, uint64_t *level) {
  const Value *type = NULL;
  const Core *k = infer(c, t, &type);
  if (k == NULL)
    return NULL;
  if (type == NULL || type->kind != VAL_UNIV)
    return nul(FAIL(c, "TYPE_MISMATCH", "expected a type, found a term of type %s", show(c, type)));
  *level = type->nat;
  return k;
}

/* Compares the inferred *TYPE with HINT. */
static const Core *finish(Checker *c, const Core *k, const Value *hint, const Value **type) {
  if (k == NULL || *type == NULL)
    return NULL;
  if (hint != NULL && !expect(c, *type, hint))
    return NULL;
  *type = hint != NULL ? hint : *type;
  return k;
}

/* 1 when T is Option A, List A or Sum E A. *ELEM is A. */
static int carrier_of(const Value *t, Carrier *carrier, const Value **elem) {
  if (val_is(t, OP_OPTION) || val_is(t, OP_LIST)) {
    *carrier = val_is(t, OP_OPTION) ? CARRIER_OPTION : CARRIER_LIST;
    *elem = t->args[0];
    return 1;
  }
  if (!val_is(t, OP_SUM))
    return 0;
  *carrier = CARRIER_SUM;
  *elem = t->args[1];
  return 1;
}

/* SHAPE (Option, List or Sum E) with the element ELEM. */
static const Value *carried(Checker *c, const Value *shape, const Value *elem) {
  if (shape == NULL)
    return NULL;
  return val_is(shape, OP_SUM) ? tyop(c, OP_SUM, 2, shape->args[0], elem, NULL) : tyop(c, shape->op, 1, elem, NULL, NULL);
}

/* Checks T as a function from DOM. *RESULT is WANT, or the result type that
   T infers when WANT is NULL. */
static const Core *fn_into(Checker *c, const Term *t, const Value *dom, const Value *want, const Value **result) {
  const Value *ft = NULL;
  const Core *f;
  if (want != NULL) {
    *result = want;
    return check(c, t, val_arrow(c->m, dom, want));
  }
  f = infer(c, t, &ft);
  if (f == NULL)
    return NULL;
  if (ft == NULL || ft->kind != VAL_PI)
    return nul(FAIL(c, "TYPE_NOT_FUNCTION", "expected a function, found a term of type %s", show(c, ft)));
  *result = closure_apply(c->m, ft, val_var(c->m, c->level));
  /* Compare at two fresh arguments before the result leaves the binder.
     Reusing its level in expect would capture a dependent result. */
  if (!conv_values(c->m, c->level + 2u, *result,
                   closure_apply(c->m, ft, val_var(c->m, c->level + 1u))))
    return nul(FAIL(c, "TYPE_MISMATCH", "this operation needs a function with a non-dependent result type"));
  return expect(c, ft, val_arrow(c->m, dom, *result)) ? f : NULL;
}

/* The argument type of the function T, from its binder or its type. */
static const Value *lam_domain(Checker *c, const Term *t) {
  uint64_t level = 0;
  const Value *ft = NULL;
  if (t->kind == TERM_LAM && t->left != NULL)
    return here(c, check_type(c, t->left, &level));
  return try_infer(c, t, &ft) != NULL && ft != NULL && ft->kind == VAL_PI ? ft->dom : NULL;
}

/* The carried argument ARG of map, bind or filter. When ARG does not infer
   (`inr 3`), its type comes from the expected type and from the binder of
   the function FN. */
static const Core *carried_arg(Call *k, const Term *arg, const Term *fn, const Value **type) {
  Checker *c = k->c;
  Carrier carrier = CARRIER_OPTION;
  const Value *elem = NULL;
  const Value *dom;
  const Core *x = try_infer(c, arg, type);
  if (x != NULL)
    return x;
  if (!carrier_of(k->hint, &carrier, &elem))
    return infer(c, arg, type);
  dom = lam_domain(c, fn);
  if (dom == NULL)
    return infer(c, arg, type);
  *type = carried(c, k->hint, dom);
  return check(c, arg, *type);
}

static const Core *need_type(Call *k, const char *what) {
  return nul(FAIL(k->c, "TYPE_INFER", "%s needs a declared type %s here", k->b->name, what));
}

static const Core *not_carrier(Call *k, const Value *type) {
  return nul(FAIL(k->c, "TYPE_MISMATCH", "%s needs a value of type Option A, List A or Sum E A, found a term of type %s", k->b->name, show(k->c, type)));
}

/* Infers the argument I of K as a path. */
static const Core *infer_path(Call *k, uint32_t i, const Value **type) {
  const Core *p = infer(k->c, k->args[i], type);
  if (p == NULL)
    return NULL;
  return *type != NULL && (*type)->kind == VAL_PATHP ? p : nul(FAIL(k->c, "TYPE_MISMATCH", "%s needs a path of type PathP L a b, found a term of type %s", k->b->name, show(k->c, *type)));
}

static const Core *rule_type1(Call *k) {
  uint64_t level = 0;
  const Core *a = check_type(k->c, k->args[0], &level);
  k->type = val_univ(k->c->m, level);
  return op3(k->c, k->b->op, 0, 1, a, NULL, NULL);
}

static const Core *rule_type2(Call *k) {
  uint64_t la = 0;
  uint64_t lb = 0;
  const Core *a = check_type(k->c, k->args[0], &la);
  const Core *b = a == NULL ? NULL : check_type(k->c, k->args[1], &lb);
  k->type = val_univ(k->c->m, la > lb ? la : lb);
  return op3(k->c, k->b->op, 0, 2, a, b, NULL);
}

/* The line of PathP: `fun (i : I) => A` or `fun i => A`, with A a type of
   *LEVEL. The core is A, under the interval binder. */
static const Core *check_line(Checker *c, const Term *t, uint64_t *level) {
  const Core *body;
  if (t->kind != TERM_LAM || (t->left != NULL && !is_interval(c, t->left)))
    return nul(FAIL(c, "TYPE_MISMATCH", "the line of PathP must be an interval abstraction fun (i : I) => A"));
  if (!push_interval(c, t->name))
    return NULL;
  body = check_type(c, t->right, level);
  pop(c);
  return body;
}

/* PathP (fun i => A) a b, with a : A[i0] and b : A[i1] (rule R12). The
   level of A is the level of the path type. */
static const Core *rule_pathp(Call *k) {
  Checker *c = k->c;
  uint64_t level = 0;
  const Core *line = check_line(c, k->args[0], &level);
  const Core *a = line == NULL ? NULL : check(c, k->args[1], at(c, line, val_iend(c->m, 0)));
  const Core *b = a == NULL ? NULL : check(c, k->args[2], at(c, line, val_iend(c->m, 1)));
  k->type = val_univ(c->m, level);
  return core_ends(c, CORE_PATHP, k->args[0]->name, NULL, line, a, b);
}

/* Path A a b is PathP (fun _ => A) a b. */
static const Core *rule_path(Call *k) {
  Checker *c = k->c;
  uint64_t level = 0;
  const Core *t = check_type(c, k->args[0], &level);
  const Value *tv = here(c, t);
  const Core *a = check(c, k->args[1], tv);
  const Core *b = a == NULL ? NULL : check(c, k->args[2], tv);
  k->type = val_univ(c->m, level);
  return core_ends(c, CORE_PATHP, "_", NULL, b == NULL ? NULL : quote_value(c->m, c->level + 1u, tv), a, b);
}

static const Core *rule_pair(Call *k) {
  Checker *c = k->c;
  const Value *ta = val_is(k->hint, OP_PROD) ? k->hint->args[0] : NULL;
  const Value *tb = val_is(k->hint, OP_PROD) ? k->hint->args[1] : NULL;
  const Core *a = ta != NULL ? check(c, k->args[0], ta) : infer(c, k->args[0], &ta);
  const Core *b = a == NULL ? NULL : (tb != NULL ? check(c, k->args[1], tb) : infer(c, k->args[1], &tb));
  k->type = tyop(c, OP_PROD, 2, ta, tb, NULL);
  return op3(c, OP_PAIR, 0, 2, a, b, NULL);
}

static const Core *rule_inj(Call *k) {
  if (!val_is(k->hint, OP_SUM))
    return need_type(k, "Sum A B");
  k->type = k->hint;
  return op3(k->c, k->b->op, 0, 1, check(k->c, k->args[0], k->hint->args[k->b->part]), NULL, NULL);
}

static const Core *rule_empty(Call *k) {
  if (!val_is(k->hint, k->b->result))
    return need_type(k, k->b->result == OP_OPTION ? "Option A" : "List A");
  k->type = k->hint;
  return op3(k->c, k->b->op, 0, 0, NULL, NULL, NULL);
}

static const Core *rule_some(Call *k) {
  Checker *c = k->c;
  const Value *a = val_is(k->hint, OP_OPTION) ? k->hint->args[0] : NULL;
  const Core *x = a != NULL ? check(c, k->args[0], a) : infer(c, k->args[0], &a);
  k->type = tyop(c, OP_OPTION, 1, a, NULL, NULL);
  return op3(c, OP_SOME, 0, 1, x, NULL, NULL);
}

static const Core *rule_cons(Call *k) {
  Checker *c = k->c;
  const Value *a = val_is(k->hint, OP_LIST) ? k->hint->args[0] : NULL;
  const Core *x = a != NULL ? check(c, k->args[0], a) : infer(c, k->args[0], &a);
  const Value *list = tyop(c, OP_LIST, 1, a, NULL, NULL);
  const Core *xs = x == NULL ? NULL : check(c, k->args[1], list);
  k->type = list;
  return op3(c, OP_CONS, 0, 2, x, xs, NULL);
}

static const Core *rule_pack(Call *k) {
  Checker *c = k->c;
  const Value *s = k->hint;
  const Core *w;
  const Core *p;
  if (s == NULL || s->kind != VAL_SIGMA)
    return need_type(k, "Sigma (x : A) B");
  w = check(c, k->args[0], s->dom);
  p = w == NULL ? NULL : check(c, k->args[1], closure_apply(c->m, s, here(c, w)));
  k->type = s;
  return op3(c, OP_PACK, 0, 2, w, p, NULL);
}

/* refl against PathP L a b is fun _ => a. The line must be constant. */
static const Core *rule_refl(Call *k) {
  Checker *c = k->c;
  const Value *e = k->hint;
  if (e == NULL || e->kind != VAL_PATHP)
    return need_type(k, "PathP L a b");
  if (!conv_values(c->m, c->level + 1u, closure_apply(c->m, e, val_ivar(c->m, c->level)), closure_apply(c->m, e, val_iend(c->m, 0))))
    return nul(FAIL(c, "TYPE_MISMATCH", "refl needs a path type with a constant line, found %s", show(c, e)));
  if (!path_ends(c, e, e->args[0], e->args[0]))
    return NULL;
  k->type = e;
  return core_bind(c, CORE_PLAM, "_", NULL, quote_value(c->m, c->level + 1u, e->args[0]));
}

static const Core *rule_part(Call *k) {
  Checker *c = k->c;
  const Value *pt = NULL;
  const Core *p = infer(c, k->args[0], &pt);
  if (p == NULL)
    return NULL;
  if (!val_is(pt, OP_PROD))
    return nul(FAIL(c, "TYPE_MISMATCH", "%s needs a pair, found a term of type %s", k->b->name, show(c, pt)));
  k->type = pt->args[k->b->part];
  return op3(c, k->b->op, 0, 1, p, NULL, NULL);
}

static const Core *rule_sigma_part(Call *k) {
  Checker *c = k->c;
  const Value *st = NULL;
  const Core *s = infer(c, k->args[0], &st);
  const Core *w;
  if (s == NULL)
    return NULL;
  if (st == NULL || st->kind != VAL_SIGMA)
    return nul(FAIL(c, "TYPE_MISMATCH", "%s needs a Sigma value, found a term of type %s", k->b->name, show(c, st)));
  w = op3(c, OP_WITNESS, 0, 1, s, NULL, NULL);
  k->type = k->b->part == 0u ? st->dom : closure_apply(c->m, st, here(c, w));
  return k->b->part == 0u ? w : op3(c, OP_PAYLOAD, 0, 1, s, NULL, NULL);
}

/* either f g s */
static const Core *rule_either(Call *k) {
  Checker *c = k->c;
  const Value *st = NULL;
  const Value *res = NULL;
  const Core *s = infer(c, k->args[2], &st);
  const Core *f;
  const Core *g;
  if (s == NULL)
    return NULL;
  if (!val_is(st, OP_SUM))
    return nul(FAIL(c, "TYPE_MISMATCH", "either needs a value of type Sum A B, found a term of type %s", show(c, st)));
  f = fn_into(c, k->args[0], st->args[0], k->hint, &res);
  g = f == NULL ? NULL : check(c, k->args[1], val_arrow(c->m, st->args[1], res));
  k->type = res;
  return op3(c, OP_EITHER, 0, 3, f, g, s);
}

/* option z f o */
static const Core *rule_option(Call *k) {
  Checker *c = k->c;
  const Value *ot = NULL;
  const Value *res = k->hint;
  const Core *o = infer(c, k->args[2], &ot);
  const Core *z;
  const Core *f;
  if (o == NULL)
    return NULL;
  if (!val_is(ot, OP_OPTION))
    return nul(FAIL(c, "TYPE_MISMATCH", "option needs a value of type Option A, found a term of type %s", show(c, ot)));
  z = res != NULL ? check(c, k->args[0], res) : infer(c, k->args[0], &res);
  f = z == NULL ? NULL : check(c, k->args[1], val_arrow(c->m, ot->args[0], res));
  k->type = res;
  return op3(c, OP_OPTION_ELIM, 0, 3, z, f, o);
}

static const Core *rule_pure(Call *k) {
  Carrier carrier = CARRIER_OPTION;
  const Value *a = NULL;
  if (!carrier_of(k->hint, &carrier, &a))
    return need_type(k, "Option A, List A or Sum E A");
  k->type = k->hint;
  return op3(k->c, OP_PURE, carrier, 1, check(k->c, k->args[0], a), NULL, NULL);
}

/* map f m */
static const Core *rule_map(Call *k) {
  Checker *c = k->c;
  Carrier carrier = CARRIER_OPTION;
  Carrier hint_carrier = CARRIER_OPTION;
  const Value *mt = NULL;
  const Value *a = NULL;
  const Value *b = NULL;
  const Value *hint_elem = NULL;
  const Core *x = carried_arg(k, k->args[1], k->args[0], &mt);
  const Core *f;
  if (x == NULL)
    return NULL;
  if (!carrier_of(mt, &carrier, &a))
    return not_carrier(k, mt);
  f = fn_into(c, k->args[0], a, carrier_of(k->hint, &hint_carrier, &hint_elem) ? hint_elem : NULL, &b);
  k->type = carried(c, mt, b);
  return op3(c, OP_MAP, carrier, 2, f, x, NULL);
}

/* bind m f */
static const Core *rule_bind(Call *k) {
  Checker *c = k->c;
  Carrier carrier = CARRIER_OPTION;
  Carrier result_carrier = CARRIER_OPTION;
  const Value *mt = NULL;
  const Value *a = NULL;
  const Value *b = NULL;
  const Value *res = NULL;
  const Core *x = carried_arg(k, k->args[0], k->args[1], &mt);
  const Core *f;
  if (x == NULL)
    return NULL;
  if (!carrier_of(mt, &carrier, &a))
    return not_carrier(k, mt);
  f = fn_into(c, k->args[1], a, k->hint, &res);
  if (f == NULL)
    return NULL;
  if (!carrier_of(res, &result_carrier, &b) || result_carrier != carrier)
    return nul(FAIL(c, "TYPE_MISMATCH", "bind needs a function into the carrier of %s, found one into %s", show(c, mt), show(c, res)));
  if (carrier == CARRIER_SUM && !conv_values(c->m, c->level, res->args[0], mt->args[0]))
    return nul(FAIL(c, "TYPE_MISMATCH", "bind must preserve the Sum error type %s, found %s", show(c, mt->args[0]), show(c, res->args[0])));
  k->type = res;
  return op3(c, OP_BIND, carrier, 2, x, f, NULL);
}

/* filter p m */
static const Core *rule_filter(Call *k) {
  Checker *c = k->c;
  Carrier carrier = CARRIER_OPTION;
  const Value *mt = NULL;
  const Value *a = NULL;
  const Core *x = carried_arg(k, k->args[1], k->args[0], &mt);
  const Core *p;
  if (x == NULL)
    return NULL;
  if (!carrier_of(mt, &carrier, &a) || carrier == CARRIER_SUM)
    return nul(FAIL(c, "TYPE_MISMATCH", "filter needs a value of type Option A or List A, found a term of type %s", show(c, mt)));
  p = check(c, k->args[0], val_arrow(c->m, a, flag_type(c)));
  k->type = mt;
  return op3(c, OP_FILTER, carrier, 2, p, x, NULL);
}

/* fold step z n (Nat) or fold f z xs (List A). */
static const Core *fold_simple(Call *k, const Core *s, const Value *st) {
  Checker *c = k->c;
  const Value *res = k->hint;
  const Core *z = res != NULL ? check(c, k->args[1], res) : infer(c, k->args[1], &res);
  int list = val_is(st, OP_LIST);
  const Value *step = list ? val_arrow(c->m, st->args[0], val_arrow(c->m, res, res)) : val_arrow(c->m, res, res);
  const Core *f = z == NULL ? NULL : check(c, k->args[0], step);
  k->used = 3u;
  k->type = res;
  return op3(c, list ? OP_FOLD_LIST : OP_FOLD_NAT, 0, 3, f, z, s);
}

/* The Pi telescope of the fold case of CTOR around BODY, a core at the
   level c->level + field_count: one argument for each field (a recursive
   field arrives folded, so its type is RES). Field k is the variable at
   level c->level + k, its type at the parameters ENV and the earlier
   fields (S4 item 13). A path field `Path F x y` arrives folded, so its
   type is `Path RES x' y'` (S4 item 14). */
static const Core *case_fields(Checker *c, const CtorInfo *ctor, const Env *env, const Value *res, const Core *body) {
  Machine *m = c->m;
  uint32_t n = ctor->field_count;
  const Core **types = arena_alloc(m->arena, ((size_t)n + 1u) * sizeof *types);
  const Core *t = body;
  uint32_t i;
  if (body == NULL)
    return NULL;
  if (types == NULL)
    return nul(FAIL(c, "OOM", "out of memory"));
  for (i = 0; i < n; i++) {
    const FieldInfo *field = &ctor->fields[i];
    uint32_t at = c->level + i;
    if (field->over_recursive && !field->path)
      return nul(FAIL(c, "HIT_BOUNDARY", "the fold of %s has no case type for the field %s: a field over an earlier recursive field must be Path F x y, with x and y earlier recursive fields", ctor->name, field->name));
    types[i] = field->path ? core_ends(c, CORE_PATHP, "_", NULL, quote_value(m, at + 1u, res), quote_value(m, at, val_var(m, c->level + field->ends[0])), quote_value(m, at, val_var(m, c->level + field->ends[1]))) : quote_value(m, at, field->recursive ? res : eval_core(m, env, field->type));
    env = env_push(m, env, val_var(m, c->level + i));
    if (types[i] == NULL || env == NULL)
      return NULL;
  }
  for (i = n; i > 0u && t != NULL; i--)
    t = core_bind(c, CORE_PI, ctor->fields[i - 1u].name, types[i - 1u], t);
  return t;
}

/* The type of the fold case of CTOR: the fields, then RES. */
static const Value *case_type(Checker *c, const CtorInfo *ctor, const Env *env, const Value *res) {
  return here(c, case_fields(c, ctor, env, res, quote_value(c->m, c->level + ctor->field_count, res)));
}

/* The fold image of V, a boundary value of a path constructor of FAM at the
   field variables (S4 R4 (a)). A field variable (a level from c->level up)
   is its folded variable; a point constructor is its case, one of the DONE
   cases in CORES, at the images of its fields. Path fields are mapped
   under a fresh interval binder. ENV and LEVEL include the boundary
   fields and surrounding interval binders to preserve captures. */
static const Value *fold_image(Checker *c, uint32_t fam, const Core *const *cores, uint32_t done, const Value *v, const Env *env, uint32_t level) {
  Machine *m = c->m;
  const FamilyInfo *f = &m->families[fam];
  const CtorInfo *ci;
  const Value *acc;
  uint32_t i;
  if (v == NULL)
    return NULL;
  if (v->kind == VAL_VAR && v->nat >= c->level)
    return v;
  /* p @ r for a path field p (dimension 2, S4 item 15): the folded p has
     the same level, and its ends are earlier recursive fields. */
  if (v->kind == VAL_PAPP && v->dom != NULL && v->dom->kind == VAL_VAR && v->dom->nat >= c->level)
    return v;
  if (!val_is(v, OP_CTOR) || v->inst < f->first_ctor || v->inst - f->first_ctor >= done || m->ctors[v->inst].interval_count != 0u)
    return vnul(FAIL(c, "HIT_BOUNDARY", "the fold cannot map the boundary term %s: a boundary term must be a field, a path field at an interval, or an earlier point constructor", show(c, v)));
  ci = &m->ctors[v->inst];
  acc = here(c, cores[v->inst - f->first_ctor]);
  for (i = 0; i < ci->field_count && acc != NULL; i++) {
    const FieldInfo *field = &ci->fields[i];
    const Value *arg = v->args[f->param_count + i];
    if (field->recursive)
      arg = fold_image(c, fam, cores, done, arg, env, level);
    else if (field->path) {
      const Value *r = val_ivar(m, level);
      const Env *inner = env_push(m, env, r);
      const Value *at = inner == NULL ? NULL : path_apply(m, arg, r, v->args[f->param_count + field->ends[0]], v->args[f->param_count + field->ends[1]]);
      const Value *image = at == NULL ? NULL : fold_image(c, fam, cores, done, at, inner, level + 1u);
      arg = image == NULL ? NULL : val_closure(m, VAL_PLAM, "_", NULL, env, quote_value(m, level + 1u, image));
    }
    acc = arg == NULL ? NULL : apply_value(m, acc, arg);
  }
  return acc;
}

/* The fold image of the constructor INST at the parameters of ST, the
   field variables (field k at level c->level + k) and the interval values
   R0 (and R1 at dimension 2). At an end it is the image of the boundary. */
static const Value *case_image(Checker *c, uint32_t inst, const Value *st, const Core *const *cores, uint32_t done, const Value *r0, const Value *r1, uint32_t level) {
  Machine *m = c->m;
  const CtorInfo *ctor = &m->ctors[inst];
  uint32_t p = st->argc;
  uint32_t n = ctor->field_count;
  uint32_t d = ctor->interval_count;
  const Value **a = arena_alloc(m->arena, ((size_t)p + n + d) * sizeof *a);
  const Env *env = c->env;
  uint32_t i;
  if (a == NULL)
    return vnul(FAIL(c, "OOM", "out of memory"));
  for (i = 0; i < p; i++)
    a[i] = st->args[i];
  for (i = 0; i < n; i++) {
    a[p + i] = val_var(m, c->level + i);
    env = env_push(m, env, a[p + i]);
    if (env == NULL)
      return NULL;
  }
  for (i = c->level + n; i < level; i++) {
    env = env_push(m, env, val_ivar(m, i));
    if (env == NULL)
      return NULL;
  }
  a[p + n] = r0;
  if (d == 2u)
    a[p + n + 1u] = r1;
  return fold_image(c, ctor->family, cores, done, val_ctor(m, inst, a, p + n + d), env, level);
}

/* The square type of the fold case of INST at dimension 2 (S4 item 15), a
   core at L = c->level + field_count:
   PathP (fun (i : I) => PathP (fun (_ : I) => RES) X(i) Y(i)) P Q,
   with X(i) and Y(i) the images at (i, i0) and (i, i1), and P and Q the
   paths fun (j : I) => the image at (i0, j) and (i1, j). For squash0 it is
   Path (Path RES x' y') p' q' (with path eta, R11), so the case type is
   isSet RES (R8 (a)). */
static const Core *square_type(Checker *c, uint32_t inst, const Value *st, const Value *res, const Core *const *cores, uint32_t done) {
  Machine *m = c->m;
  uint32_t l = c->level + m->ctors[inst].field_count;
  const Value *i = val_ivar(m, l);
  const Core *x = quote_value(m, l + 1u, case_image(c, inst, st, cores, done, i, val_iend(m, 0), l + 1u));
  const Core *y = x == NULL ? NULL : quote_value(m, l + 1u, case_image(c, inst, st, cores, done, i, val_iend(m, 1), l + 1u));
  const Core *inner = y == NULL ? NULL : core_ends(c, CORE_PATHP, "_", NULL, quote_value(m, l + 2u, res), x, y);
  const Core *ends[2];
  int side;
  for (side = 0; side < 2; side++) {
    ends[side] = inner == NULL ? NULL : core_bind(c, CORE_PLAM, "_", NULL, quote_value(m, l + 1u, case_image(c, inst, st, cores, done, val_iend(m, side), i, l + 1u)));
    if (ends[side] == NULL)
      return NULL;
  }
  return core_ends(c, CORE_PATHP, "_", NULL, inner, ends[0], ends[1]);
}

/* The type of the fold case of the path constructor INST: (folded fields)
   -> PathP (fun (_ : I) => RES) B0 B1 at dimension 1, with B0 and B1 the
   fold images of the boundary at i0 and i1, or the square type at
   dimension 2. Field k is the variable at level c->level + k: the field in
   the boundary, its folded value in the image. */
static const Value *path_case_type(Checker *c, uint32_t inst, const Value *st, const Value *res, const Core *const *cores, uint32_t done) {
  Machine *m = c->m;
  const CtorInfo *ctor = &m->ctors[inst];
  const Env *env = env_of(c, st->args, st->argc);
  uint32_t l = c->level + ctor->field_count;
  const Core *ends[2];
  const Core *t;
  int side;
  if (ctor->interval_count != 1u && ctor->interval_count != 2u)
    return vnul(FAIL(c, "HIT_BOUNDARY", "a fold of %s, a constructor with %u interval fields, is not in S4: S4 folds dimensions 1 and 2", ctor->name, (unsigned)ctor->interval_count));
  if (ctor->interval_count == 2u)
    return here(c, case_fields(c, ctor, env, res, square_type(c, inst, st, res, cores, done)));
  for (side = 0; side < 2; side++)
    ends[side] = quote_value(m, l, case_image(c, inst, st, cores, done, val_iend(m, side), NULL, l));
  t = ends[0] == NULL || ends[1] == NULL ? NULL : core_ends(c, CORE_PATHP, "_", NULL, quote_value(m, l + 1u, res), ends[0], ends[1]);
  return here(c, case_fields(c, ctor, env, res, t));
}

/* fold case... x over a domain family: one case for each constructor. */
static const Core *fold_family(Call *k, const Core *s, const Value *st) {
  Checker *c = k->c;
  Machine *m = c->m;
  const FamilyInfo *f = &m->families[st->inst];
  const Env *env = env_of(c, st->args, st->argc);
  const Value *res = k->hint;
  const Core **cores = arena_alloc(m->arena, ((size_t)f->ctor_count + 2u) * sizeof *cores);
  uint32_t j;
  if (cores == NULL)
    return nul(FAIL(c, "OOM", "out of memory"));
  const Value *ct;
  if (res == NULL && m->ctors[f->first_ctor].field_count == 0u && m->ctors[f->first_ctor].interval_count == 0u)
    cores[0] = infer(c, k->args[0], &res);
  if (res == NULL)
    return nul(FAIL(c, "TYPE_INFER", "fold over %s needs a declared result type here", f->name));
  for (j = 0; j < f->ctor_count; j++) {
    ct = cores[j] != NULL ? NULL : m->ctors[f->first_ctor + j].interval_count == 0u ? case_type(c, &m->ctors[f->first_ctor + j], env, res) : path_case_type(c, f->first_ctor + j, st, res, cores, j);
    cores[j] = cores[j] != NULL ? cores[j] : ct == NULL ? NULL : check(c, k->args[j], ct);
    if (cores[j] == NULL)
      return NULL;
  }
  /* The result type RES goes before the scrutinee: the fold of a formal
     hcomp is an hcomp in RES (S4 R3 (a)). */
  cores[f->ctor_count] = quote_value(m, c->level, res);
  cores[f->ctor_count + 1u] = s;
  if (cores[f->ctor_count] == NULL)
    return NULL;
  k->used = f->ctor_count + 1u;
  k->type = res;
  return core_op(c, OP_FOLD_FAMILY, st->inst, 0, cores, f->ctor_count + 2u);
}

static int simple_scrutinee(const Value *t) {
  return val_is(t, OP_NAT) || val_is(t, OP_LIST);
}

/* fold takes its scrutinee last, so the scrutinee type sets the arity. */
static const Core *rule_fold(Call *k) {
  Checker *c = k->c;
  const Value *st = NULL;
  const Core *s = try_infer(c, k->args[k->argc - 1u], &st);
  uint32_t fam = val_is(st, OP_FAMILY) ? st->inst : NO_FAMILY;
  if (s != NULL && fam != NO_FAMILY && c->m->families[fam].ctor_count + 1u == k->argc) {
    k->hint = k->outer;
    return fold_family(k, s, st);
  }
  if (s != NULL && k->argc == 3u && simple_scrutinee(st)) {
    k->hint = k->outer;
    return fold_simple(k, s, st);
  }
  s = k->argc > 3u ? try_infer(c, k->args[2], &st) : NULL;
  if (s != NULL && simple_scrutinee(st)) {
    k->hint = NULL;
    return fold_simple(k, s, st);
  }
  s = infer(c, k->args[k->argc - 1u], &st);
  return s == NULL ? NULL : nul(FAIL(c, "TYPE_MISMATCH", "fold needs a last argument of type Nat, List A or a domain family, and one case for each constructor; found %u arguments and a last argument of type %s", k->argc, show(c, st)));
}

/* unfold g limit seed, with g : S -> Option (Prod A S). */
static const Core *rule_unfold(Call *k) {
  Checker *c = k->c;
  const Value *st = NULL;
  const Value *a;
  const Core *seed;
  const Core *limit;
  const Core *g;
  if (!val_is(k->hint, OP_LIST))
    return need_type(k, "List A");
  a = k->hint->args[0];
  seed = infer(c, k->args[2], &st);
  limit = seed == NULL ? NULL : check(c, k->args[1], nat_type(c));
  g = limit == NULL ? NULL : check(c, k->args[0], val_arrow(c->m, st, tyop(c, OP_OPTION, 1, tyop(c, OP_PROD, 2, a, st, NULL), NULL, NULL)));
  k->type = k->hint;
  return op3(c, OP_UNFOLD, 0, 3, g, limit, seed);
}

/* The binder name of a path abstraction made from a path type: the name of
   its line, or `i` for the constant line `_` of Path. The printed term must
   name the variable that its body uses. */
static const char *path_binder(const char *name) {
  return name == NULL || strcmp(name, "_") == 0 ? "i" : name;
}

/* sym p is (fun q => fun i => q @ ~i) p, of type PathP (fun i => L[~i]) b a
   for p : PathP L a b. The application keeps p outside the interval
   binder, so p needs no shift. */
static const Core *rule_sym(Call *k) {
  Checker *c = k->c;
  const Value *pt = NULL;
  const Core *p = infer_path(k, 0, &pt);
  const Core *flip;
  const Core *line;
  const Core *body;
  if (p == NULL)
    return NULL;
  flip = core_iop(c, CORE_INEG, core_index(c, CORE_VAR, 0), NULL);
  line = quote_value(c->m, c->level + 1u, closure_apply(c->m, pt, at(c, flip, val_ivar(c->m, c->level))));
  k->type = here(c, core_ends(c, CORE_PATHP, pt->name, NULL, line, quote_value(c->m, c->level, pt->args[1]), quote_value(c->m, c->level, pt->args[0])));
  body = core_ends(c, CORE_PAPP, NULL, core_index(c, CORE_VAR, 1), flip, quote_value(c->m, c->level + 2u, pt->args[0]), quote_value(c->m, c->level + 2u, pt->args[1]));
  return core_app(c, core_bind(c, CORE_LAM, "q", NULL, core_bind(c, CORE_PLAM, path_binder(pt->name), NULL, body)), p);
}

/* cong f p is (fun g => fun q => fun i => g (q @ i)) f p, for
   f : (x : A) -> B and p : Path A a b, of type
   PathP (fun i => B[p @ i]) (f a) (f b). */
static const Core *rule_cong(Call *k) {
  Checker *c = k->c;
  const Value *pt = NULL;
  const Value *ft = NULL;
  const Value *i = val_ivar(c->m, c->level);
  const Core *p = infer_path(k, 1, &pt);
  const Core *f = p == NULL ? NULL : try_infer(c, k->args[0], &ft);
  const Value *fv;
  const Value *pv;
  const Core *line;
  const Core *body;
  if (p == NULL)
    return NULL;
  if (f == NULL) {
    ft = k->hint != NULL && k->hint->kind == VAL_PATHP ? val_arrow(c->m, closure_apply(c->m, pt, val_iend(c->m, 0)), closure_apply(c->m, k->hint, val_iend(c->m, 0))) : NULL;
    f = ft != NULL ? check(c, k->args[0], ft) : infer(c, k->args[0], &ft);
  }
  if (f == NULL)
    return NULL;
  if (ft == NULL || ft->kind != VAL_PI)
    return nul(FAIL(c, "TYPE_NOT_FUNCTION", "cong needs a function, found a term of type %s", show(c, ft)));
  if (!conv_values(c->m, c->level + 1u, closure_apply(c->m, pt, i), ft->dom))
    return nul(FAIL(c, "TYPE_MISMATCH", "cong needs a path in %s, found a term of type %s", show(c, ft->dom), show(c, pt)));
  fv = here(c, f);
  pv = here(c, p);
  line = quote_value(c->m, c->level + 1u, closure_apply(c->m, ft, path_apply(c->m, pv, i, pt->args[0], pt->args[1])));
  k->type = here(c, core_ends(c, CORE_PATHP, pt->name, NULL, line, quote_value(c->m, c->level, apply_value(c->m, fv, pt->args[0])), quote_value(c->m, c->level, apply_value(c->m, fv, pt->args[1]))));
  body = core_app(c, core_index(c, CORE_VAR, 2), core_ends(c, CORE_PAPP, NULL, core_index(c, CORE_VAR, 1), core_index(c, CORE_VAR, 0), quote_value(c->m, c->level + 3u, pt->args[0]), quote_value(c->m, c->level + 3u, pt->args[1])));
  return core_app(c, core_app(c, core_bind(c, CORE_LAM, "g", NULL, core_bind(c, CORE_LAM, "q", NULL, core_bind(c, CORE_PLAM, path_binder(pt->name), NULL, body))), f), p);
}

/* funExt h is (fun g => fun i => fun x => g x @ i) h, for
   h : (x : A) -> PathP L a b, of type
   PathP (fun i => (x : A) -> L[i]) (fun x => a) (fun x => b). */
static const Core *rule_funext(Call *k) {
  Checker *c = k->c;
  const Value *ht = NULL;
  const Core *h = infer(c, k->args[0], &ht);
  const char *x;
  const Value *cod;
  const Value *at_x;
  const Value *inner;
  const Core *line;
  const Core *body;
  if (h == NULL)
    return NULL;
  cod = ht != NULL && ht->kind == VAL_PI ? closure_apply(c->m, ht, val_var(c->m, c->level)) : NULL;
  if (cod == NULL || cod->kind != VAL_PATHP)
    return nul(FAIL(c, "TYPE_MISMATCH", "funExt needs a function into paths (x : A) -> PathP L a b, found a term of type %s", show(c, ht)));
  x = ht->name != NULL ? ht->name : "x";
  /* The line: i at LEVEL, x at LEVEL + 1. */
  at_x = closure_apply(c->m, ht, val_var(c->m, c->level + 1u));
  /* The body: h at LEVEL, i at LEVEL + 1, x at LEVEL + 2. */
  inner = closure_apply(c->m, ht, val_var(c->m, c->level + 2u));
  if (at_x == NULL || inner == NULL || at_x->kind != VAL_PATHP || inner->kind != VAL_PATHP)
    return nul(FAIL(c, "INTERNAL", "funExt: the codomain of %s is not a path type at each argument", show(c, ht)));
  line = core_bind(c, CORE_PI, x, quote_value(c->m, c->level + 1u, ht->dom), quote_value(c->m, c->level + 2u, closure_apply(c->m, at_x, val_ivar(c->m, c->level))));
  k->type = here(c, core_ends(c, CORE_PATHP, cod->name, NULL, line, core_bind(c, CORE_LAM, x, NULL, quote_value(c->m, c->level + 1u, cod->args[0])), core_bind(c, CORE_LAM, x, NULL, quote_value(c->m, c->level + 1u, cod->args[1]))));
  body = core_ends(c, CORE_PAPP, NULL, core_app(c, core_index(c, CORE_VAR, 2), core_index(c, CORE_VAR, 0)), core_index(c, CORE_VAR, 1), quote_value(c->m, c->level + 3u, inner->args[0]), quote_value(c->m, c->level + 3u, inner->args[1]));
  return core_app(c, core_bind(c, CORE_LAM, "g", NULL, core_bind(c, CORE_PLAM, path_binder(cod->name), NULL, core_bind(c, CORE_LAM, x, NULL, body))), h);
}

/* TRANSP_LINE: the line names its interval variable in the message. */
static const Core *transp_line_fail(Checker *c, const char *name, const Value *face, const Core *line) {
  const Value *v = at(c, line, val_ivar(c->m, c->level));
  const char *f = show(c, face);
  const char *zero = show(c, at(c, line, val_iend(c->m, 0)));
  const char *bound;
  if (!push_interval(c, path_binder(name)))
    return NULL;
  bound = show(c, v);
  pop(c);
  return nul(FAIL(c, "TRANSP_LINE", "the line of transp must be constant where %s holds, but it is %s at %s and %s at i0", f, bound, path_binder(name), zero));
}

/* transp (fun i => A) phi a, with phi : I and a : A[i0], of type A[i1]
   (S2 R3 (a)). On each face of phi the line is constant: under the face
   map of each conjunction, A at a fresh i is A at i0. */
static const Core *rule_transp(Call *k) {
  Checker *c = k->c;
  uint64_t level = 0;
  const Core *line = check_line(c, k->args[0], &level);
  const Core *phi = line == NULL ? NULL : check(c, k->args[1], interval_type(c));
  const Core *a = phi == NULL ? NULL : check(c, k->args[2], at(c, line, val_iend(c->m, 0)));
  const Value *face = a == NULL ? NULL : face_norm(c->m, here(c, phi));
  const Env *e = NULL;
  uint32_t j;
  if (face == NULL)
    return NULL;
  for (j = 0; j < face->conj_count; j++) {
    if (!env_face(c->m, c->env, &face->conj[j], &e))
      return NULL;
    if (!conv_values(c->m, c->level + 1u, eval_core(c->m, env_push(c->m, e, val_ivar(c->m, c->level)), line), eval_core(c->m, env_push(c->m, e, val_iend(c->m, 0)), line)))
      return transp_line_fail(c, k->args[0]->name, face, line);
  }
  k->type = at(c, line, val_iend(c->m, 1));
  return op3(c, OP_TRANSP, 0, 3, core_bind(c, CORE_PLAM, path_binder(k->args[0]->name), NULL, line), phi, a);
}

/* A branch `k -> t` of a system: the core of its key, the face normal
   form of the key and the core of its body (under the tube binder). */
typedef struct {
  const Term *term;
  const Core *key;
  const Value *face;
  const Core *body;
} Branch;

static uint32_t system_size(const Term *sys) {
  uint32_t n = 0;
  for (; sys != NULL && sys->left != NULL; sys = sys->right)
    n++;
  return n;
}

/* The body core BODY at the interval value R, in the env E. */
static const Value *branch_at(Checker *c, const Env *e, const Core *body, const Value *r) {
  const Env *with = env_push(c->m, e, r);
  return with == NULL ? NULL : eval_core(c->m, with, body);
}

/* Split disjunctive keys before elaboration: inferred arguments can be
   different on each conjunction, so each one needs its own body core. */
static Branch *split_branches(Checker *c, const Branch *bs, uint32_t n, uint32_t *count) {
  Branch *out;
  Value *face;
  uint32_t i;
  uint32_t j;
  uint32_t k = 0;
  *count = 0;
  for (i = 0; i < n; i++)
    *count += bs[i].face->conj_count > 0 ? bs[i].face->conj_count : 1u;
  out = arena_alloc(c->m->arena, ((size_t)*count + 1u) * sizeof *out);
  if (out == NULL) {
    FAIL(c, "OOM", "out of memory");
    return NULL;
  }
  for (i = 0; i < n; i++) {
    if (bs[i].face->conj_count <= 1u) {
      out[k++] = bs[i];
      continue;
    }
    for (j = 0; j < bs[i].face->conj_count; j++) {
      face = arena_alloc(c->m->arena, sizeof *face);
      if (face == NULL) {
        FAIL(c, "OOM", "out of memory");
        return NULL;
      }
      *face = *bs[i].face;
      face->conj = &bs[i].face->conj[j];
      face->conj_count = 1;
      out[k] = bs[i];
      out[k].face = face;
      out[k].key = quote_value(c->m, c->level, face);
      if (out[k].key == NULL)
        return NULL;
      k++;
    }
  }
  return out;
}

/* The body of B against A, with the tube binder NAME, under its single
   conjunction. A key with no conjunction checks in the plain context.
   NAME is NULL for a binder that no term can use (Glue and glue). */
static const Core *hcomp_branch(Checker *c, const char *name, const Core *a, const Branch *b) {
  const Env *saved = c->env;
  const Scope *scope = c->scope;
  const Scope *s;
  Scope *copy;
  const Scope **tail;
  const Scope *acted;
  const Core *type;
  const Env *e;
  const Core *core = NULL;
  const Core *one;
  uint32_t n = b->face->conj_count > 0 ? b->face->conj_count : 1u;
  uint32_t j;
  for (j = 0; j < n; j++) {
    e = saved;
    if (b->face->conj_count > 0 && !env_face(c->m, saved, &b->face->conj[j], &e))
      return NULL;
    acted = NULL;
    tail = &acted;
    for (s = scope; s != NULL; s = s->next) {
      type = quote_value(c->m, c->level, s->type);
      copy = arena_alloc(c->m->arena, sizeof *copy);
      if (type == NULL || copy == NULL)
        return nul(c->m->diag->set ? 0 : FAIL(c, "OOM", "out of memory"));
      *copy = *s;
      copy->type = eval_core(c->m, e, type);
      if (copy->type == NULL)
        return NULL;
      *tail = copy;
      tail = &copy->next;
    }
    *tail = NULL;
    c->scope = acted;
    c->env = e;
    if (!push_interval(c, name)) {
      c->env = saved;
      c->scope = scope;
      return NULL;
    }
    one = check(c, b->term->right, eval_core(c->m, e, a));
    pop(c);
    c->env = saved;
    c->scope = scope;
    if (one == NULL)
      return NULL;
    core = core == NULL ? one : core;
  }
  return core;
}

/* SYSTEM_OVERLAP: the branches X and Y agree at a fresh i on each
   conjunction of JOINT, the meet of their faces. */
static int hcomp_overlap(Checker *c, const char *name, const Value *joint, const Branch *x, const Branch *y) {
  const Value *i = val_ivar(c->m, c->level);
  const Env *e = NULL;
  const Value *vx;
  const Value *vy;
  const char *sx;
  const char *sy;
  uint32_t j;
  if (joint == NULL)
    return 0;
  for (j = 0; j < joint->conj_count; j++) {
    if (!env_face(c->m, c->env, &joint->conj[j], &e))
      return 0;
    vx = branch_at(c, e, x->body, i);
    vy = branch_at(c, e, y->body, i);
    if (vx == NULL || vy == NULL)
      return 0;
    if (!conv_values(c->m, c->level + 1u, vx, vy)) {
      if (!push_interval(c, path_binder(name)))
        return 0;
      sx = show(c, vx);
      sy = show(c, vy);
      pop(c);
      return FAIL(c, "SYSTEM_OVERLAP", "the branches for %s and %s must agree where both keys hold, but they are %s and %s", show(c, here(c, x->key)), show(c, here(c, y->key)), sx, sy);
    }
  }
  return 1;
}

/* HCOMP_CAP: on each conjunction of the face of B, B at i0 is the base U0. */
static int hcomp_cap(Checker *c, const Core *u0, const Branch *b) {
  const Env *e = NULL;
  const Value *at0;
  const Value *base;
  uint32_t j;
  for (j = 0; j < b->face->conj_count; j++) {
    if (!env_face(c->m, c->env, &b->face->conj[j], &e))
      return 0;
    at0 = branch_at(c, e, b->body, val_iend(c->m, 0));
    base = eval_core(c->m, e, u0);
    if (at0 == NULL || base == NULL)
      return 0;
    if (!conv_values(c->m, c->level, at0, base))
      return FAIL(c, "HCOMP_CAP", "the base of hcomp must be the branch for %s at i0, but the base is %s and the branch is %s at i0", show(c, here(c, b->key)), show(c, base), show(c, at0));
  }
  return 1;
}

/* The branches of the system body of TUBE. Each key is checked against I
   and face-normalized. The join of the keys must be the face PHI
   (SYSTEM_COVER). A disjunctive key gives a branch for each conjunction
   (split_branches). *COUNT gets the number of branches. hcomp, Glue and
   glue use this. */
static Branch *system_branches(Checker *c, const Term *tube, const Value *phi, uint32_t *count) {
  uint32_t n = system_size(tube->right);
  Branch *bs = arena_alloc(c->m->arena, ((size_t)n + 1u) * sizeof *bs);
  const Value *join = val_iend(c->m, 0);
  const Value *pf;
  const Term *cell;
  uint32_t i;
  if (bs == NULL) {
    FAIL(c, "OOM", "out of memory");
    return NULL;
  }
  for (cell = tube->right, i = 0; i < n; cell = cell->right, i++) {
    bs[i].term = cell->left;
    bs[i].key = check(c, cell->left->left, interval_type(c));
    bs[i].face = face_norm(c->m, here(c, bs[i].key));
    if (bs[i].face == NULL)
      return NULL;
    join = face_join(c->m, join, bs[i].face);
  }
  pf = face_norm(c->m, phi);
  if (join == NULL || pf == NULL)
    return NULL;
  if (!conv_values(c->m, c->level, join, pf)) {
    FAIL(c, "SYSTEM_COVER", "the keys of the system must cover %s, but their join is %s", show(c, pf), show(c, join));
    return NULL;
  }
  return split_branches(c, bs, n, count);
}

/* SYSTEM_OVERLAP for each pair of the N checked branches BS. */
static int system_overlaps(Checker *c, const char *name, const Branch *bs, uint32_t n) {
  uint32_t i;
  uint32_t j;
  for (i = 0; i < n; i++)
    for (j = i + 1u; j < n; j++)
      if (!hcomp_overlap(c, name, face_meet(c->m, bs[i].face, bs[j].face), &bs[i], &bs[j]))
        return 0;
  return 1;
}

/* The OP_SYSTEM core of the N checked branches BS: each key, then its body
   under the path binder NAME. */
static const Core *system_core(Checker *c, const char *name, const Branch *bs, uint32_t n) {
  const Core **sys = arena_alloc(c->m->arena, (2u * (size_t)n + 1u) * sizeof *sys);
  uint32_t i;
  if (sys == NULL)
    return nul(FAIL(c, "OOM", "out of memory"));
  for (i = 0; i < n; i++) {
    sys[2u * i] = bs[i].key;
    sys[2u * i + 1u] = core_bind(c, CORE_PLAM, name, NULL, bs[i].body);
  }
  return core_op(c, OP_SYSTEM, 0, 0, sys, 2u * n);
}

/* hcomp A phi (fun i => [ k0 -> t0, ... ]) u0, with phi : I, each key
   k : I in the outer context, u0 : A and each branch t : A for i : I where
   its key holds (S2 R2 (a)), of type A. The keys cover phi (SYSTEM_COVER),
   two branches agree where both keys hold (SYSTEM_OVERLAP) and each branch
   at i0 is u0 (HCOMP_CAP). Hcomp at a universe is Glue (S3). A face acts
   on both the env values and the local scope types. */
static const Core *rule_hcomp(Call *k) {
  Checker *c = k->c;
  uint64_t level = 0;
  const Term *tube = k->args[2];
  const Core *a = check_type(c, k->args[0], &level);
  const Value *av = here(c, a);
  const Core *phi;
  const Core *u0;
  const Core *args[4];
  Branch *bs;
  uint32_t n = 0;
  uint32_t i;
  if (av == NULL)
    return NULL;
  if (av->kind == VAL_UNIV && av->nat != 0u)
    return nul(FAIL(c, "HCOMP_UNIVERSE", "hcomp at Type %llu needs Glue above Type 0", (unsigned long long)av->nat));
  phi = check(c, k->args[1], interval_type(c));
  if (phi == NULL)
    return NULL;
  if (tube->kind != TERM_LAM || (tube->left != NULL && !is_interval(c, tube->left)) || tube->right == NULL || tube->right->kind != TERM_SYSTEM)
    return nul(FAIL(c, "TYPE_MISMATCH", "the tube of hcomp must be a system abstraction fun (i : I) => [ phi -> t, ... ]"));
  bs = system_branches(c, tube, here(c, phi), &n);
  if (bs == NULL)
    return NULL;
  for (i = 0; i < n; i++) {
    bs[i].body = hcomp_branch(c, tube->name, a, &bs[i]);
    if (bs[i].body == NULL)
      return NULL;
  }
  if (!system_overlaps(c, tube->name, bs, n))
    return NULL;
  u0 = check(c, k->args[3], av);
  if (u0 == NULL)
    return NULL;
  for (i = 0; i < n; i++)
    if (!hcomp_cap(c, u0, &bs[i]))
      return NULL;
  args[0] = a;
  args[1] = phi;
  args[2] = system_core(c, path_binder(tube->name), bs, n);
  args[3] = u0;
  k->type = av;
  return core_op(c, OP_HCOMP, 0, 0, args, 4);
}

/* GLUE_BINDER: the tube of Glue and glue is fun (_ : I) => [ k -> t, ... ].
   The binder must be _, so a branch cannot use it and an outer i stays
   visible. The branches check under a binder with no name (see
   glue_branch), so a branch cannot use the name _ for it. */
static int glue_tube(Call *k, const Term *tube) {
  Checker *c = k->c;
  if (tube->kind != TERM_LAM || (tube->left != NULL && !is_interval(c, tube->left)) || tube->right == NULL || tube->right->kind != TERM_SYSTEM)
    return FAIL(c, "TYPE_MISMATCH", "the tube of %s must be a system abstraction fun (_ : I) => [ phi -> t, ... ]", k->b->name);
  if (tube->name != NULL && strcmp(tube->name, "_") != 0)
    return FAIL(c, "GLUE_BINDER", "the tube binder of %s must be _, found %s; a branch of %s cannot use the tube variable", k->b->name, tube->name, k->b->name);
  return 1;
}

/* Sigma (T : Type 0) (Equiv T A), the type of a Glue branch, for the type
   AV in the current context. Equiv is the domain definition. */
static const Core *equiv_sigma(Checker *c, const Value *av) {
  uint32_t d = find_def(c->m, "Equiv");
  const Core *equiv;
  if (d == NO_DEF)
    return nul(FAIL(c, "NAME_UNKNOWN", "Glue needs the domain definition Equiv, which is not declared"));
  equiv = core_app(c, core_app(c, core_index(c, CORE_GLOBAL, d), core_index(c, CORE_VAR, 0)), quote_value(c->m, c->level + 1u, av));
  return core_bind(c, CORE_SIGMA, "T", core_leaf(c, CORE_UNIV, 0), equiv);
}

/* 1 for OOM and for a failure of the evaluator. These failures are not
   about a branch, so GLUE_EQUIV does not replace them. */
static int kept_failure(const char *code) {
  return strcmp(code, "OOM") == 0 || strcmp(code, "INTERNAL") == 0 || strcmp(code, "EVAL_FUEL") == 0 || strcmp(code, "EVAL_DEPTH") == 0;
}

/* GLUE_EQUIV: the branch B of Glue A phi sys is pack T e of type SIGMA,
   that is Sigma (T : Type 0) (Equiv T A), where its key holds. A failure
   of this check becomes GLUE_EQUIV, with the first code and message. */
static const Core *glue_branch(Checker *c, const Core *sigma, const Value *av, const Branch *b) {
  char inner[sizeof c->m->diag->msg];
  const char *code;
  Diag clean = *c->m->diag;
  const Core *body = hcomp_branch(c, NULL, sigma, b);
  if (body != NULL || !c->m->diag->set)
    return body;
  code = c->m->diag->code != NULL ? c->m->diag->code : "-";
  if (kept_failure(code))
    return NULL;
  snprintf(inner, sizeof inner, "%s", c->m->diag->msg);
  *c->m->diag = clean;
  return nul(FAIL(c, "GLUE_EQUIV", "the branch for %s must be pack T e with T : Type 0 and e : Equiv T %s: %s: %s", show(c, here(c, b->key)), show(c, av), code, inner));
}

/* Glue A phi (fun (_ : I) => [ k0 -> pack T0 e0, ... ]), with A : Type 0,
   phi : I, each key k : I in the outer context and each branch of type
   Sigma (T : Type 0) (Equiv T A) where its key holds (S3b), of type
   Type 0. The keys cover phi (SYSTEM_COVER), each branch is an
   equivalence into A (GLUE_EQUIV) and two branches agree where both keys
   hold (SYSTEM_OVERLAP). The core is OP_GLUE (A, phi, sys), with sys as
   hcomp keeps it. */
static const Core *rule_glue(Call *k) {
  Checker *c = k->c;
  const Term *tube = k->args[2];
  const Core *a = check(c, k->args[0], val_univ(c->m, 0));
  const Value *av = here(c, a);
  const Core *phi = av == NULL ? NULL : check(c, k->args[1], interval_type(c));
  const Core *sigma;
  const Core *args[3];
  Branch *bs;
  uint32_t n = 0;
  uint32_t i;
  if (phi == NULL || !glue_tube(k, tube))
    return NULL;
  sigma = equiv_sigma(c, av);
  bs = sigma == NULL ? NULL : system_branches(c, tube, here(c, phi), &n);
  if (bs == NULL)
    return NULL;
  for (i = 0; i < n; i++) {
    bs[i].body = glue_branch(c, sigma, av, &bs[i]);
    if (bs[i].body == NULL)
      return NULL;
  }
  if (!system_overlaps(c, "_", bs, n))
    return NULL;
  args[0] = a;
  args[1] = phi;
  args[2] = system_core(c, "_", bs, n);
  k->type = val_univ(c->m, 0);
  return core_op(c, OP_GLUE, 0, 0, args, 3);
}

/* GLUE_BOUNDARY: on each conjunction of the face of B, unglue of the
   branch t of glue is the base A0. G is the expected Glue type. A key of G
   holds there, so unglue t is witness e t (S3b face rule). */
static int glue_boundary(Checker *c, const Value *g, const Core *a0, const Branch *b) {
  const Core *args[4];
  const Core *un;
  const Env *e = NULL;
  const Value *vu;
  const Value *va;
  uint32_t j;
  for (j = 0; j < 3u; j++)
    args[j] = quote_value(c->m, c->level + 1u, g->args[j]);
  args[3] = b->body;
  un = core_op(c, OP_UNGLUE, 0, 0, args, 4);
  if (un == NULL)
    return 0;
  for (j = 0; j < b->face->conj_count; j++) {
    if (!env_face(c->m, c->env, &b->face->conj[j], &e))
      return 0;
    vu = branch_at(c, e, un, val_iend(c->m, 1));
    va = eval_core(c->m, e, a0);
    if (vu == NULL || va == NULL)
      return 0;
    if (!conv_values(c->m, c->level, vu, va))
      return FAIL(c, "GLUE_BOUNDARY", "unglue of the branch for %s must be the base of glue where its key holds, but it is %s and the base is %s", show(c, here(c, b->key)), show(c, vu), show(c, va));
  }
  return 1;
}

/* glue (fun (_ : I) => [ k0 -> t0, ... ]) a against Glue A phi sys (S3b),
   in check mode only. The keys cover phi (SYSTEM_COVER), each branch t is
   a term of the Glue type where its key holds, that is of T, two branches
   agree where both keys hold (SYSTEM_OVERLAP), a : A and unglue t is a
   where the key of t holds (GLUE_BOUNDARY). The core is OP_GLUE_ELEM (phi,
   sys, a). */
static const Core *rule_glue_elem(Call *k) {
  Checker *c = k->c;
  const Value *g = k->hint;
  const Term *tube = k->args[0];
  const Core *gt;
  const Core *a;
  const Core *args[3];
  Branch *bs;
  uint32_t n = 0;
  uint32_t i;
  if (!val_is(g, OP_GLUE) || g->argc != 3u)
    return need_type(k, "Glue A phi sys");
  if (!glue_tube(k, tube))
    return NULL;
  gt = quote_value(c->m, c->level, g);
  bs = gt == NULL ? NULL : system_branches(c, tube, g->args[1], &n);
  if (bs == NULL)
    return NULL;
  for (i = 0; i < n; i++) {
    bs[i].body = hcomp_branch(c, NULL, gt, &bs[i]);
    if (bs[i].body == NULL)
      return NULL;
  }
  if (!system_overlaps(c, "_", bs, n))
    return NULL;
  a = check(c, k->args[1], g->args[0]);
  if (a == NULL)
    return NULL;
  for (i = 0; i < n; i++)
    if (!glue_boundary(c, g, a, &bs[i]))
      return NULL;
  args[0] = quote_value(c->m, c->level, g->args[1]);
  args[1] = system_core(c, "_", bs, n);
  args[2] = a;
  k->type = g;
  return core_op(c, OP_GLUE_ELEM, 0, 0, args, 3);
}

/* unglue g, with g of the type Glue A phi sys (S3b), of type A. The core
   keeps A, phi and sys, as p @ r keeps its ends (S1), because a neutral
   carries no type. The type of g must be a Glue type (UNGLUE_TYPE). The
   core is OP_UNGLUE (A, phi, sys, g). */
static const Core *rule_unglue(Call *k) {
  Checker *c = k->c;
  const Value *gt = NULL;
  const Core *g = infer(c, k->args[0], &gt);
  const Core *args[4];
  uint32_t i;
  if (g == NULL)
    return NULL;
  if (!val_is(gt, OP_GLUE) || gt->argc != 3u)
    return nul(FAIL(c, "UNGLUE_TYPE", "unglue needs a term of a type Glue A phi sys, found a term of type %s", show(c, gt)));
  for (i = 0; i < 3u; i++)
    args[i] = quote_value(c->m, c->level, gt->args[i]);
  args[3] = g;
  k->type = gt->args[0];
  return core_op(c, OP_UNGLUE, 0, 0, args, 4);
}

/* The result type of a NAT2 or FIN_OP builtin. FIN is Fin n of the
   argument n (NULL for NAT2). */
static const Value *op_result(Checker *c, const Builtin *b, const Value *fin) {
  if (b->result != OP_FIN)
    return tyop(c, b->result, 0, NULL, NULL, NULL);
  return b->part == 0u ? fin : tyop(c, OP_FIN, 1, val_nat(c->m, b->part), NULL, NULL);
}

/* natAdd x y, ..., natEq x y (Flag), succ x: each argument : Nat. */
static const Core *rule_nat2(Call *k) {
  Checker *c = k->c;
  const Core *x = check(c, k->args[0], nat_type(c));
  const Core *y = x == NULL || k->b->arity < 2u ? NULL : check(c, k->args[1], nat_type(c));
  k->type = op_result(c, k->b, NULL);
  return op3(c, k->b->op, 0, k->b->arity, x, y, NULL);
}

/* Fin n: n : Nat, Fin n : Type 0. */
static const Core *rule_fin(Call *k) {
  const Core *n = check(k->c, k->args[0], nat_type(k->c));
  k->type = val_univ(k->c->m, 0);
  return op3(k->c, OP_FIN, 0, 1, n, NULL, NULL);
}

/* finAdd n a b, finNeg n a, finMul n a b, finVal n a, finEq n a b: n :
   Nat, each other argument : Fin n. The result is Fin n, Nat (finVal) or
   Fin 2 (finEq). */
static const Core *rule_fin_op(Call *k) {
  Checker *c = k->c;
  const Core *n = check(c, k->args[0], nat_type(c));
  const Value *fin = n == NULL ? NULL : tyop(c, OP_FIN, 1, here(c, n), NULL, NULL);
  const Core *x = fin == NULL ? NULL : check(c, k->args[1], fin);
  const Core *y = x == NULL || k->b->arity < 3u ? NULL : check(c, k->args[2], fin);
  k->type = op_result(c, k->b, fin);
  return op3(c, k->b->op, 0, k->b->arity, n, x, y);
}

/* flagIf b yes no */
static const Core *rule_flag_if(Call *k) {
  Checker *c = k->c;
  const Value *res = k->hint;
  const Core *b = check(c, k->args[0], flag_type(c));
  const Core *yes = b == NULL ? NULL : (res != NULL ? check(c, k->args[1], res) : infer(c, k->args[1], &res));
  const Core *no = yes == NULL ? NULL : check(c, k->args[2], res);
  k->type = res;
  return op3(c, OP_FLAG_IF, 0, 3, b, yes, no);
}

/* absurd A e (K2): A : Type n, e : Empty; the result is A. The value is
   stuck, because Empty has no closed value. */
static const Core *rule_absurd(Call *k) {
  Checker *c = k->c;
  uint64_t level = 0;
  const Core *a = check_type(c, k->args[0], &level);
  const Core *e = a == NULL ? NULL : check(c, k->args[1], tyop(c, OP_EMPTY, 0, NULL, NULL, NULL));
  if (e == NULL)
    return NULL;
  k->type = here(c, a);
  return op3(c, OP_ABSURD, 0, 2, a, e, NULL);
}

/* ENV with the entry at index IX set to V: the entries before IX are
   copied, the tail is shared. */
static const Env *env_with(Machine *m, const Env *env, uint32_t ix, const Value *v) {
  const Env *rest;
  if (env == NULL)
    return NULL;
  if (ix == 0u)
    return env_push(m, env->next, v);
  rest = env_with(m, env->next, ix - 1u, v);
  return rest == NULL ? NULL : env_push(m, rest, env->value);
}

/* Refresh the types of locals introduced after KEY in the branch env. */
static const Scope *fin_case_scope(Checker *c, const Scope *scope, const Scope *key) {
  Scope *out;
  const Core *type;
  if (scope == NULL || scope->level <= key->level)
    return scope;
  out = arena_alloc(c->m->arena, sizeof *out);
  type = quote_value(c->m, c->level, scope->type);
  if (out == NULL || type == NULL)
    return NULL;
  *out = *scope;
  out->type = here(c, type);
  out->next = fin_case_scope(c, scope->next, key);
  return out->type == NULL || out->next == NULL ? NULL : out;
}

/* Case I of finCase at the hint *WANT. When *WANT is NULL, the case is
   inferred and its type goes in *WANT (the hint of the later cases). When
   KEY is a local, the hint and the case are checked with KEY bound to the
   literal I (the finAll rule). */
static const Core *fin_case_arm(Checker *c, const Term *t, const Value **want, const Scope *key, uint64_t i) {
  const Env *saved = c->env;
  const Scope *saved_scope = c->scope;
  const Core *wq = *want == NULL || key == NULL ? NULL : quote_value(c->m, c->level, *want);
  const Env *bound = key == NULL ? saved : (*want != NULL && wq == NULL ? NULL : env_with(c->m, saved, c->level - 1u - key->level, val_nat(c->m, i)));
  const Core *out;
  /* A NULL env is a failure only when a key is bound: with no key, the env
     stays as it is, and the top-level env is empty (S5a item 5). */
  if (key != NULL && bound == NULL)
    return c->m->diag->set ? NULL : nul(FAIL(c, "OOM", "out of memory"));
  c->env = bound;
  if (key != NULL)
    c->scope = fin_case_scope(c, saved_scope, key);
  if (key != NULL && c->scope == NULL)
    out = c->m->diag->set ? NULL : nul(FAIL(c, "OOM", "out of memory"));
  else
    out = *want == NULL ? infer(c, t, want) : check(c, t, wq == NULL ? *want : here(c, wq));
  c->scope = saved_scope;
  c->env = saved;
  return out;
}

/* finCase n k c0 .. c(n-1) (S5a, R12): the surface form of OP_FIN_CASE. n
   is a literal Nat, k : Fin n, and the case count is n (else FIN_CASE). The
   cases take the rest of the spine. With no hint, the type of case 0 is the
   type of each case. finCase 0 k is ex falso. The core is OP_FIN_CASE (k,
   c0, ..., c(n-1)). */
static const Core *rule_fin_case(Call *k) {
  Checker *c = k->c;
  const Value *res = k->outer;
  const Core *n = check(c, k->args[0], nat_type(c));
  const Value *nv = here(c, n);
  const Value *fin = nv == NULL ? NULL : tyop(c, OP_FIN, 1, nv, NULL, NULL);
  const Core *key = fin == NULL ? NULL : check(c, k->args[1], fin);
  const Scope *local = key != NULL && k->args[1]->kind == TERM_VAR ? find_local(c, k->args[1]->name) : NULL;
  uint32_t count = k->argc - 2u;
  const Core **cores = arena_alloc(c->m->arena, ((size_t)count + 1u) * sizeof *cores);
  uint32_t i;
  k->used = k->argc;
  if (key == NULL)
    return NULL;
  if (cores == NULL)
    return nul(FAIL(c, "OOM", "out of memory"));
  if (nv->kind != VAL_NAT)
    return nul(FAIL(c, "FIN_CASE", "finCase needs a literal case count, found %s", show(c, nv)));
  if (nv->nat != (uint64_t)count)
    return nul(FAIL(c, "FIN_CASE", "finCase %llu needs %llu cases, found %u", (unsigned long long)nv->nat, (unsigned long long)nv->nat, count));
  if (res == NULL && count == 0u)
    return nul(FAIL(c, "TYPE_INFER", "finCase 0 needs an expected type"));
  cores[0] = key;
  for (i = 0; i < count; i++) {
    cores[i + 1u] = fin_case_arm(c, k->args[i + 2u], &res, local, i);
    if (cores[i + 1u] == NULL)
      return NULL;
  }
  k->type = res;
  {
    Core *out = (Core *)core_op(c, OP_FIN_CASE, 0, 0, cores, count + 1u);
    if (out == NULL)
      return NULL;
    out->a = quote_value(c->m, c->level, res);
    return out->a == NULL ? NULL : out;
  }
}

/* decEqFin n a b and decEqNat a b (S5b item 7): n : Nat, a and b : Fin n
   (decEqFin) or : Nat (decEqNat). The type is the value of `Sum (Path T a
   b) ((Path T a b) -> Empty)`, T the type of a and b. It converts with the
   domain `Dec (Path T a b)`. The line of the path is T quoted under the
   binder, as in rule_path. */
static const Core *rule_dec_eq(Call *k) {
  Checker *c = k->c;
  int fin = k->b->op == OP_DEC_EQ_FIN;
  const Core *n = fin ? check(c, k->args[0], nat_type(c)) : NULL;
  const Value *t = fin ? (n == NULL ? NULL : tyop(c, OP_FIN, 1, here(c, n), NULL, NULL)) : nat_type(c);
  const Core *x = t == NULL ? NULL : check(c, k->args[fin ? 1u : 0u], t);
  const Core *y = x == NULL ? NULL : check(c, k->args[fin ? 2u : 1u], t);
  const Core *path = y == NULL ? NULL : core_ends(c, CORE_PATHP, "_", NULL, quote_value(c->m, c->level + 1u, t), x, y);
  const Core *empty = path == NULL ? NULL : op3(c, OP_EMPTY, 0, 0, NULL, NULL, NULL);
  const Core *dec = path == NULL ? NULL : op3(c, OP_SUM, 0, 2, path, core_bind(c, CORE_PI, "_", path, empty), NULL);
  k->type = here(c, dec);
  return fin ? op3(c, k->b->op, 0, 3, n, x, y) : op3(c, k->b->op, 0, 2, x, y, NULL);
}

static const Core *rule(Call *k) {
  switch (k->b->rule) {
  case RULE_TYPE0:
    k->type = val_univ(k->c->m, 0);
    return op3(k->c, k->b->op, 0, 0, NULL, NULL, NULL);
  case RULE_VALUE0:
    k->type = tyop(k->c, k->b->result, 0, NULL, NULL, NULL);
    return op3(k->c, k->b->op, 0, 0, NULL, NULL, NULL);
  case RULE_TYPE1:
    return rule_type1(k);
  case RULE_TYPE2:
    return rule_type2(k);
  case RULE_INTERVAL:
    return nul(FAIL(k->c, "INTERVAL_TYPE", "I is the interval, not a type; only a path abstraction or a PathP line can bind an interval variable (rule R9)"));
  case RULE_IEND:
    k->type = interval_type(k->c);
    return new_core(k->c, k->b->part == 0u ? CORE_IZERO : CORE_IONE);
  case RULE_PATHP:
    return rule_pathp(k);
  case RULE_PATH:
    return rule_path(k);
  case RULE_PAIR:
    return rule_pair(k);
  case RULE_INJ:
    return rule_inj(k);
  case RULE_EMPTY:
    return rule_empty(k);
  case RULE_SOME:
    return rule_some(k);
  case RULE_CONS:
    return rule_cons(k);
  case RULE_PACK:
    return rule_pack(k);
  case RULE_REFL:
    return rule_refl(k);
  case RULE_PART:
    return rule_part(k);
  case RULE_SIGMA_PART:
    return rule_sigma_part(k);
  case RULE_EITHER:
    return rule_either(k);
  case RULE_OPTION:
    return rule_option(k);
  case RULE_PURE:
    return rule_pure(k);
  case RULE_MAP:
    return rule_map(k);
  case RULE_BIND:
    return rule_bind(k);
  case RULE_FILTER:
    return rule_filter(k);
  case RULE_FOLD:
    return rule_fold(k);
  case RULE_UNFOLD:
    return rule_unfold(k);
  case RULE_SYM:
    return rule_sym(k);
  case RULE_CONG:
    return rule_cong(k);
  case RULE_FUNEXT:
    return rule_funext(k);
  case RULE_TRANSP:
    return rule_transp(k);
  case RULE_HCOMP:
    return rule_hcomp(k);
  case RULE_GLUE:
    return rule_glue(k);
  case RULE_GLUE_ELEM:
    return rule_glue_elem(k);
  case RULE_UNGLUE:
    return rule_unglue(k);
  case RULE_FLAG_IF:
    return rule_flag_if(k);
  case RULE_ABSURD:
    return rule_absurd(k);
  case RULE_NAT2:
    return rule_nat2(k);
  case RULE_FIN_OP:
    return rule_fin_op(k);
  case RULE_FIN_CASE:
    return rule_fin_case(k);
  case RULE_DEC_EQ:
    return rule_dec_eq(k);
  case RULE_FIN:
    return rule_fin(k);
  }
  return NULL;
}

/* Checks the family parameters ARGS. VALS gets their values. */
static int check_params(Checker *c, const FamilyInfo *f, const Term *const *args, const Core **cores, const Value **vals) {
  uint32_t i;
  for (i = 0; i < f->param_count; i++) {
    cores[i] = check(c, args[i], eval_core(c->m, env_of(c, vals, i), f->param_types[i]));
    vals[i] = here(c, cores[i]);
    if (vals[i] == NULL)
      return 0;
  }
  return 1;
}

static const Core *family_app(Call *k, uint32_t fam) {
  Checker *c = k->c;
  const FamilyInfo *f = &c->m->families[fam];
  const Core **cores = arena_alloc(c->m->arena, ((size_t)f->param_count + 1u) * sizeof *cores);
  const Value **vals = arena_alloc(c->m->arena, ((size_t)f->param_count + 1u) * sizeof *vals);
  if (cores == NULL || vals == NULL)
    return nul(FAIL(c, "OOM", "out of memory"));
  if (!check_params(c, f, k->args, cores, vals))
    return NULL;
  k->type = val_univ(c->m, f->universe);
  return core_op(c, OP_FAMILY, fam, 0, cores, f->param_count);
}

/* A constructor takes the family parameters (the indices), then its fields,
   then its interval arguments (a path constructor). */
static const Core *ctor_app(Call *k, uint32_t ctor) {
  Checker *c = k->c;
  const CtorInfo *ci = &c->m->ctors[ctor];
  const FamilyInfo *f = &c->m->families[ci->family];
  uint32_t p = f->param_count;
  uint32_t q = p + ci->field_count;
  uint32_t n = q + ci->interval_count;
  const Core **cores = arena_alloc(c->m->arena, ((size_t)n + 1u) * sizeof *cores);
  const Value **vals = arena_alloc(c->m->arena, ((size_t)p + 1u) * sizeof *vals);
  const Env *env;
  uint32_t j;
  if (cores == NULL || vals == NULL)
    return nul(FAIL(c, "OOM", "out of memory"));
  if (!check_params(c, f, k->args, cores, vals))
    return NULL;
  env = env_of(c, vals, p);
  /* Field j is checked at the parameters and the earlier field values. */
  for (j = 0; j < ci->field_count; j++) {
    const Value *fv;
    cores[p + j] = check(c, k->args[p + j], eval_core(c->m, env, ci->fields[j].type));
    fv = here(c, cores[p + j]);
    env = fv == NULL ? NULL : env_push(c->m, env, fv);
    if (env == NULL)
      return NULL;
  }
  for (j = q; j < n; j++) {
    cores[j] = check(c, k->args[j], interval_type(c));
    if (cores[j] == NULL)
      return NULL;
  }
  k->type = val_op(c->m, VAL_OP, OP_FAMILY, ci->family, 0, vals, p);
  return core_op(c, OP_CTOR, ctor, 0, cores, n);
}

/* A field of a one-constructor family is a projection. */
static const Core *proj_app(Call *k, uint32_t ctor, uint32_t field) {
  Checker *c = k->c;
  const CtorInfo *ci = &c->m->ctors[ctor];
  const Value *xt = NULL;
  const Core *x = infer(c, k->args[0], &xt);
  const Env *env;
  uint32_t i;
  if (x == NULL)
    return NULL;
  if (!val_is(xt, OP_FAMILY) || xt->inst != ci->family)
    return nul(FAIL(c, "TYPE_MISMATCH", "%s needs a value of the family %s, found a term of type %s", ci->fields[field].name, c->m->families[ci->family].name, show(c, xt)));
  /* The field type at the parameters and the projections of the earlier
     fields of X (S4 item 13), as the second part of a Sigma. */
  env = env_of(c, xt->args, xt->argc);
  for (i = 0; i < field; i++) {
    const Value *fv = here(c, core_op(c, OP_PROJ, ctor, i, &x, 1));
    env = fv == NULL ? NULL : env_push(c->m, env, fv);
    if (env == NULL)
      return NULL;
  }
  k->type = eval_core(c->m, env, ci->fields[field].type);
  return core_op(c, OP_PROJ, ctor, field, &x, 1);
}

static int find_known(Checker *c, const char *name, Head *h) {
  const Machine *m = c->m;
  uint32_t i;
  uint32_t j;
  for (i = 0; i < m->family_count; i++) {
    if (strcmp(m->families[i].name, name) == 0) {
      h->kind = HEAD_FAMILY;
      h->index = i;
      c->self_uses += i == c->self;
      return 1;
    }
  }
  for (i = 0; i < m->ctor_count; i++) {
    if (strcmp(m->ctors[i].name, name) == 0) {
      h->kind = HEAD_CTOR;
      h->index = i;
      return 1;
    }
  }
  for (i = 0; i < m->ctor_count; i++) {
    for (j = 0; m->families[m->ctors[i].family].ctor_count == 1u && j < m->ctors[i].field_count; j++) {
      if (strcmp(m->ctors[i].fields[j].name, name) == 0) {
        h->kind = HEAD_PROJ;
        h->index = i;
        h->field = j;
        return 1;
      }
    }
  }
  for (i = 0; i < BUILTIN_COUNT; i++) {
    if (strcmp(BUILTINS[i].name, name) == 0) {
      h->kind = HEAD_BUILTIN;
      h->builtin = &BUILTINS[i];
      return 1;
    }
  }
  return 0;
}

/* An unknown name. When it names this declaration or a later one, the use
   is recursion (rule R3). */
static int unknown_name(Checker *c, const char *name) {
  size_t i;
  for (i = c->decl_index; i < c->decls->count; i++) {
    if (strcmp(c->decls->items[i].name, name) == 0)
      return FAIL(c, "REFUSE_REC", "%s is this definition or a later one; a definition can use only earlier ones (rule R3)", name);
  }
  return FAIL(c, "NAME_UNKNOWN", "unknown name %s", name);
}

/* Name resolution order: locals (the innermost first), definitions,
   families, constructors, projections, core names. */
static int resolve_head(Checker *c, const Term *head, Head *h) {
  memset(h, 0, sizeof *h);
  h->kind = HEAD_TERM;
  if (head->kind != TERM_VAR || find_local(c, head->name) != NULL || find_def(c->m, head->name) != NO_DEF)
    return 1;
  return find_known(c, head->name, h) ? 1 : unknown_name(c, head->name);
}

static uint32_t known_arity(const Checker *c, const Head *h) {
  const CtorInfo *ci = h->kind == HEAD_CTOR ? &c->m->ctors[h->index] : NULL;
  switch (h->kind) {
  case HEAD_TERM:
    return 0u;
  case HEAD_FAMILY:
    return c->m->families[h->index].param_count;
  case HEAD_CTOR:
    return ci == NULL ? 0u : c->m->families[ci->family].param_count + ci->field_count + ci->interval_count;
  case HEAD_PROJ:
    return 1u;
  case HEAD_BUILTIN:
    return h->builtin->arity;
  }
  return 0u;
}

static const char *known_name(const Checker *c, const Head *h) {
  switch (h->kind) {
  case HEAD_TERM:
    return "?";
  case HEAD_FAMILY:
    return c->m->families[h->index].name;
  case HEAD_CTOR:
    return c->m->ctors[h->index].name;
  case HEAD_PROJ:
    return c->m->ctors[h->index].fields[h->field].name;
  case HEAD_BUILTIN:
    return h->builtin->name;
  }
  return "?";
}

static const Core *known_core(Call *k, const Head *h) {
  switch (h->kind) {
  case HEAD_TERM:
    return nul(FAIL(k->c, "INTERNAL", "a plain term reached the core name rules"));
  case HEAD_FAMILY:
    return family_app(k, h->index);
  case HEAD_CTOR:
    return ctor_app(k, h->index);
  case HEAD_PROJ:
    return proj_app(k, h->index, h->field);
  case HEAD_BUILTIN:
    return rule(k);
  }
  return NULL;
}

/* An under-applied core name checked against a function type: WHOLE becomes
   `fun x => WHOLE x`, with a name that the lexer cannot make. */
static const Core *eta(Checker *c, const Term *whole, const Value *hint, const Value **type) {
  char *name = arena_alloc(c->m->arena, 24);
  Term *x = arena_alloc(c->m->arena, sizeof *x);
  Term *app = arena_alloc(c->m->arena, sizeof *app);
  Term *lam = arena_alloc(c->m->arena, sizeof *lam);
  if (name == NULL || x == NULL || app == NULL || lam == NULL)
    return nul(FAIL(c, "OOM", "out of memory"));
  c->eta_count++;
  snprintf(name, 24, "%%eta%u", c->eta_count);
  x->kind = TERM_VAR;
  x->name = name;
  app->kind = TERM_APP;
  app->left = whole;
  app->right = x;
  lam->kind = TERM_LAM;
  lam->name = name;
  lam->right = app;
  return check_lam(c, lam, hint, type);
}

/* Applies K of type TY to ARGS, one by one. */
static const Core *apply_rest(Checker *c, const Core *k, const Value *ty, const Term *const *args, uint32_t argc, const Value *hint, const Value **type) {
  uint32_t i;
  for (i = 0; i < argc && k != NULL; i++) {
    const Core *a;
    if (ty == NULL || ty->kind != VAL_PI)
      return nul(FAIL(c, "TYPE_NOT_FUNCTION", "a term of type %s is applied to an argument", show(c, ty)));
    a = check(c, args[i], ty->dom);
    ty = closure_apply(c->m, ty, here(c, a));
    k = core_app(c, k, a);
  }
  *type = ty;
  return finish(c, k, hint, type);
}

static const Core *var_core(Checker *c, const char *name, const Value **type) {
  const Scope *s = find_local(c, name);
  uint32_t d = find_def(c->m, name);
  if (s != NULL) {
    *type = s->type;
    if (s->level >= c->field_base && s->level < c->field_top) {
      c->field_dep = 1;
      c->field_rec |= val_is(s->type, OP_FAMILY) && s->type->inst == c->self;
    }
    return core_index(c, CORE_VAR, c->level - 1u - s->level);
  }
  if (d != NO_DEF) {
    *type = c->m->defs[d].type;
    return core_index(c, CORE_GLOBAL, d);
  }
  return nul(FAIL(c, "NAME_UNKNOWN", "unknown name %s", name));
}

static const Core *apply_term(Checker *c, const Term *head, const Term *const *args, uint32_t argc, const Value *hint, const Value **type) {
  const Value *ty = NULL;
  const Core *k = head->kind == TERM_VAR ? var_core(c, head->name, &ty) : infer(c, head, &ty);
  return apply_rest(c, k, ty, args, argc, hint, type);
}

static const Core *apply_known(Checker *c, const Term *whole, const Head *h, const Term *const *args, uint32_t argc, const Value *hint, const Value **type) {
  uint32_t arity = known_arity(c, h);
  Call k;
  const Core *core;
  if (argc < arity && hint != NULL && hint->kind == VAL_PI)
    return eta(c, whole, hint, type);
  if (argc < arity)
    return nul(FAIL(c, "TYPE_ARITY", "%s needs %u arguments, found %u", known_name(c, h), arity, argc));
  memset(&k, 0, sizeof k);
  k.c = c;
  k.b = h->builtin;
  k.args = args;
  k.argc = argc;
  k.hint = argc == arity ? hint : NULL;
  k.outer = hint;
  k.used = arity;
  core = known_core(&k, h);
  return apply_rest(c, core, k.type, args + k.used, argc - k.used, hint, type);
}

static const Core *elab_spine(Checker *c, const Term *t, const Value *hint, const Value **type) {
  const Term **args;
  const Term *head = t;
  uint32_t argc = 0;
  uint32_t i;
  Head h;
  while (head->kind == TERM_APP) {
    argc++;
    head = head->left;
  }
  if (!resolve_head(c, head, &h))
    return NULL;
  if (argc > SPINE_MAX && !(h.kind == HEAD_BUILTIN && h.builtin->rule == RULE_FIN_CASE))
    return nul(FAIL(c, "TYPE_ARITY", "an application has more than %u arguments", SPINE_MAX));
  args = arena_alloc(c->m->arena, ((size_t)argc + 1u) * sizeof *args);
  if (args == NULL)
    return nul(FAIL(c, "OOM", "out of memory"));
  for (head = t, i = argc; head->kind == TERM_APP; head = head->left)
    args[--i] = head->right;
  return h.kind == HEAD_TERM ? apply_term(c, head, args, argc, hint, type) : apply_known(c, t, &h, args, argc, hint, type);
}

static int annotation_ok(Checker *c, const Term *ann, const Value *dom) {
  uint64_t level = 0;
  const Value *v = here(c, check_type(c, ann, &level));
  if (v == NULL)
    return 0;
  return conv_values(c->m, c->level, v, dom) ? 1 : FAIL(c, "TYPE_MISMATCH", "the binder type %s differs from the expected type %s", show(c, v), show(c, dom));
}

/* A path abstraction against the path type HINT: the body at the interval
   variable, then the ends at i0 and i1. */
static const Core *check_plam(Checker *c, const Term *t, const Value *hint, const Value **type) {
  const Core *body;
  if (t->left != NULL && !is_interval(c, t->left))
    return nul(FAIL(c, "TYPE_MISMATCH", "a term of type %s is a path; its binder must be an interval variable fun (%s : I) => ...", show(c, hint), t->name));
  if (!push_interval(c, t->name))
    return NULL;
  body = check(c, t->right, closure_apply(c->m, hint, c->env->value));
  pop(c);
  if (body == NULL || !path_ends(c, hint, at(c, body, val_iend(c->m, 0)), at(c, body, val_iend(c->m, 1))))
    return NULL;
  *type = hint;
  return core_bind(c, CORE_PLAM, t->name, NULL, body);
}

/* fun (i : I) => t with no expected type: PathP (fun i => T) t[i0] t[i1]. */
static const Core *infer_plam(Checker *c, const Term *t, const Value **type) {
  const Value *bt = NULL;
  const Core *body;
  if (!push_interval(c, t->name))
    return NULL;
  body = infer(c, t->right, &bt);
  pop(c);
  if (body == NULL)
    return NULL;
  if (bt == NULL || val_is(bt, OP_INTERVAL))
    return nul(FAIL(c, "INTERVAL_TYPE", "the path fun (%s : I) => ... has values in I; the line of a path must be a type (rule R9)", t->name));
  *type = here(c, core_ends(c, CORE_PATHP, t->name, NULL, quote_value(c->m, c->level + 1u, bt), quote_value(c->m, c->level, at(c, body, val_iend(c->m, 0))), quote_value(c->m, c->level, at(c, body, val_iend(c->m, 1)))));
  return core_bind(c, CORE_PLAM, t->name, NULL, body);
}

/* The literal bound of a Fin n domain, or 0 when there is no finAll: the
   bound must leave room for the k argument in the uint32_t arity. */
static uint64_t fin_cases(const Value *dom) {
  if (!val_is(dom, OP_FIN) || dom->args[0]->kind != VAL_NAT || dom->args[0]->nat >= UINT32_MAX)
    return 0;
  return dom->args[0]->nat;
}

/* finAll (S3a): the body at each literal j below n, with the binder bound
   to j. The core is OP_FIN_CASE (k, body0, ..., body(n-1)). */
static const Core *check_fin_all(Checker *c, const Term *t, const Value *hint, uint64_t n) {
  const Core **args = arena_alloc(c->m->arena, ((size_t)n + 1u) * sizeof *args);
  char inner[sizeof c->m->diag->msg];
  const char *code;
  Diag clean = *c->m->diag;
  uint64_t j;
  if (args == NULL)
    return nul(FAIL(c, "OOM", "out of memory"));
  args[0] = core_index(c, CORE_VAR, 0);
  for (j = 0; j < n; j++) {
    const Value *x = val_nat(c->m, j);
    if (!push_value(c, t->name, hint->dom, x))
      return NULL;
    args[j + 1u] = check(c, t->right, closure_apply(c->m, hint, x));
    pop(c);
    if (args[j + 1u] == NULL) {
      code = c->m->diag->code != NULL ? c->m->diag->code : "-";
      if (strcmp(code, "OOM") == 0)
        return NULL;
      snprintf(inner, sizeof inner, "%s", c->m->diag->msg);
      *c->m->diag = clean;
      return nul(FAIL(c, "FIN_CASE", "the case %llu of Fin %llu fails: %s: %s", (unsigned long long)j, (unsigned long long)n, code, inner));
    }
  }
  {
    Core *out = (Core *)core_op(c, OP_FIN_CASE, 0, 0, args, (uint32_t)n + 1u);
    if (out == NULL)
      return NULL;
    out->a = quote_value(c->m, c->level + 1u, closure_apply(c->m, hint, val_var(c->m, c->level)));
    return out->a == NULL ? NULL : out;
  }
}

static const Core *check_lam(Checker *c, const Term *t, const Value *hint, const Value **type) {
  const Core *body;
  Diag clean;
  if (hint->kind == VAL_PATHP)
    return check_plam(c, t, hint, type);
  if (hint->kind != VAL_PI)
    return nul(FAIL(c, "TYPE_MISMATCH", "a function is not a term of type %s", show(c, hint)));
  if (t->left != NULL && !annotation_ok(c, t->left, hint->dom))
    return NULL;
  clean = *c->m->diag;
  if (!push(c, t->name, hint->dom))
    return NULL;
  body = check(c, t->right, closure_apply(c->m, hint, c->env->value));
  pop(c);
  /* finAll: the ordinary check failed on a Fin n domain (literal n), so
     check each case. The ordinary core stays when the ordinary check passes. */
  if (body == NULL && fin_cases(hint->dom) > 0 && strcmp(c->m->diag->code != NULL ? c->m->diag->code : "-", "OOM") != 0) {
    *c->m->diag = clean;
    body = check_fin_all(c, t, hint, fin_cases(hint->dom));
  }
  *type = hint;
  return core_bind(c, CORE_LAM, t->name, NULL, body);
}

static const Core *infer_lam(Checker *c, const Term *t, const Value **type) {
  uint64_t level = 0;
  const Value *dom;
  const Value *bt = NULL;
  const Core *body;
  const Core *quoted;
  if (t->left == NULL)
    return nul(FAIL(c, "TYPE_INFER", "fun %s => ... needs a binder type here", t->name));
  if (is_interval(c, t->left))
    return infer_plam(c, t, type);
  dom = here(c, check_type(c, t->left, &level));
  if (!push(c, t->name, dom))
    return NULL;
  body = infer(c, t->right, &bt);
  quoted = body == NULL ? NULL : quote_value(c->m, c->level, bt);
  pop(c);
  *type = val_closure(c->m, VAL_PI, t->name, dom, c->env, quoted);
  return core_bind(c, CORE_LAM, t->name, NULL, body);
}

/* (x : A) -> B and Sigma (x : A) B. */
static const Core *elab_binder(Checker *c, const Term *t, CoreKind kind, const Value *hint, const Value **type) {
  uint64_t la = 0;
  uint64_t lb = 0;
  const Core *a = check_type(c, t->left, &la);
  const Core *b;
  if (a == NULL || !push(c, t->name, here(c, a)))
    return NULL;
  b = check_type(c, t->right, &lb);
  pop(c);
  *type = val_univ(c->m, la > lb ? la : lb);
  return finish(c, core_bind(c, kind, t->name, a, b), hint, type);
}

/* p @ r: p : PathP L a b and r : I give L[r]. The core keeps a and b. */
static const Core *elab_papp(Checker *c, const Term *t, const Value *hint, const Value **type) {
  const Value *pt = NULL;
  const Core *p = infer(c, t->left, &pt);
  const Core *r;
  if (p == NULL)
    return NULL;
  if (pt == NULL || pt->kind != VAL_PATHP)
    return nul(FAIL(c, "TYPE_MISMATCH", "@ needs a path of type PathP L a b, found a term of type %s", show(c, pt)));
  r = check(c, t->right, interval_type(c));
  *type = r == NULL ? NULL : closure_apply(c->m, pt, here(c, r));
  return finish(c, core_ends(c, CORE_PAPP, NULL, p, r, quote_value(c->m, c->level, pt->args[0]), quote_value(c->m, c->level, pt->args[1])), hint, type);
}

/* ~r, r /\ s and r \/ s: the operands and the result are in I. */
static const Core *elab_ival(Checker *c, const Term *t, CoreKind kind, const Value *hint, const Value **type) {
  const Value *iv = interval_type(c);
  const Core *a = check(c, t->left, iv);
  const Core *b = kind == CORE_INEG || a == NULL ? NULL : check(c, t->right, iv);
  *type = iv;
  return finish(c, core_iop(c, kind, a, b), hint, type);
}

static const Core *elab_inner(Checker *c, const Term *t, const Value *hint, const Value **type) {
  switch (t->kind) {
  case TERM_VAR:
  case TERM_APP:
    return elab_spine(c, t, hint, type);
  case TERM_NAT:
    if (val_is(hint, OP_FIN) && hint->args[0]->kind == VAL_NAT) {
      if (t->nat >= hint->args[0]->nat)
        return nul(FAIL(c, "FIN_RANGE", "the literal %llu is not below %llu, so it is not a value of Fin %llu", (unsigned long long)t->nat, (unsigned long long)hint->args[0]->nat, (unsigned long long)hint->args[0]->nat));
      *type = hint;
      return core_leaf(c, CORE_NAT, t->nat);
    }
    *type = nat_type(c);
    return finish(c, core_leaf(c, CORE_NAT, t->nat), hint, type);
  case TERM_LAM:
    return hint != NULL ? check_lam(c, t, hint, type) : infer_lam(c, t, type);
  case TERM_PI:
    return elab_binder(c, t, CORE_PI, hint, type);
  case TERM_SIGMA:
    return elab_binder(c, t, CORE_SIGMA, hint, type);
  case TERM_UNIV:
    if (t->nat >= UNIVERSE_MAX)
      return nul(FAIL(c, "TYPE_UNIVERSE", "Type %llu is above the largest universe Type %u", (unsigned long long)t->nat, UNIVERSE_MAX - 1u));
    *type = val_univ(c->m, t->nat + 1u);
    return finish(c, core_leaf(c, CORE_UNIV, t->nat), hint, type);
  case TERM_PAPP:
    return elab_papp(c, t, hint, type);
  case TERM_INEG:
    return elab_ival(c, t, CORE_INEG, hint, type);
  case TERM_IMEET:
    return elab_ival(c, t, CORE_IMEET, hint, type);
  case TERM_IJOIN:
    return elab_ival(c, t, CORE_IJOIN, hint, type);
  case TERM_SYSTEM:
  case TERM_BRANCH:
    return nul(FAIL(c, "TYPE_MISMATCH", "a system [ phi -> t, ... ] is only the tube of hcomp, fun (i : I) => [ ... ]"));
  }
  return NULL;
}

static const Core *elab(Checker *c, const Term *t, const Value *hint, const Value **type) {
  const Core *k;
  if (c->depth >= CHECK_DEPTH_LIMIT)
    return nul(FAIL(c, "CHECK_DEPTH", "the term is nested deeper than %u levels", CHECK_DEPTH_LIMIT));
  c->depth++;
  k = elab_inner(c, t, hint, type);
  c->depth--;
  return k;
}

/* NULL when NAME is free, else the reason it is taken (rule R4). */
static const char *name_owner(const Checker *c, const char *name) {
  const Machine *m = c->m;
  uint32_t i;
  uint32_t j;
  for (i = 0; i < sizeof RESERVED / sizeof RESERVED[0]; i++) {
    if (strcmp(RESERVED[i], name) == 0)
      return "a reserved name";
  }
  for (i = 0; i < BUILTIN_COUNT; i++) {
    if (strcmp(BUILTINS[i].name, name) == 0)
      return "a core name";
  }
  for (i = 0; i < m->family_count; i++) {
    if (strcmp(m->families[i].name, name) == 0)
      return "a domain family";
  }
  for (i = 0; i < m->ctor_count; i++) {
    for (j = 0; j < m->ctors[i].field_count; j++) {
      if (strcmp(m->ctors[i].fields[j].name, name) == 0)
        return "a domain field";
    }
    if (strcmp(m->ctors[i].name, name) == 0)
      return "a domain constructor";
  }
  i = find_def(m, name);
  if (i == NO_DEF)
    return NULL;
  return m->defs[i].origin == ORIGIN_DOMAIN ? "a domain definition" : "already defined";
}

static int name_free(Checker *c, const char *name) {
  const char *why = name_owner(c, name);
  return why == NULL ? 1 : FAIL(c, "REFUSE_NAME", "%s is %s (rule R4)", name, why);
}

/* T is the family FAM at its own parameter variables `F p1 .. pn`, in
   the context of the parameters and SHIFT earlier fields (S4 items 9
   and 13). */
static int own_family(const Machine *m, uint32_t fam, const Core *t, uint32_t shift) {
  uint32_t p = m->families[fam].param_count;
  uint32_t k;
  if (t->kind != CORE_OP || t->op != OP_FAMILY || t->inst != fam || t->argc != p)
    return 0;
  for (k = 0; k < p; k++)
    if (t->args[k]->kind != CORE_VAR || t->args[k]->index != p - 1u - k + shift)
      return 0;
  return 1;
}

/* A recursive field must be the whole field type, the family at its own
   parameters. The EARLIER point fields are locals (S4 item 13). */
static int check_field(Checker *c, uint32_t fam, const Field *field, FieldInfo *out, uint32_t earlier) {
  uint64_t level = 0;
  const Core *t;
  if (!name_free(c, field->name))
    return 0;
  c->self_uses = 0;
  c->field_base = c->level - earlier;
  c->field_top = c->level;
  c->field_dep = 0;
  c->field_rec = 0;
  t = check_type(c, field->type, &level);
  c->field_top = c->field_base;
  if (t == NULL)
    return 0;
  if (level > c->m->families[fam].universe)
    c->m->families[fam].universe = level;
  out->name = field->name;
  out->type = t;
  /* A path type over earlier recursive fields names the family, but it is
     a path field, not a recursive field (S4 item 14). */
  out->recursive = c->self_uses > 0u && !(t->kind == CORE_PATHP && c->field_rec);
  out->dependent = c->field_dep;
  out->over_recursive = c->field_rec;
  if (out->recursive && !own_family(c->m, fam, t, earlier))
    return FAIL(c, "DOMAIN_FAMILY", "the recursive field %s must have the type %s at its own parameters", field->name, c->m->families[fam].name);
  return 1;
}

/* The field EARLIER of FIELDS is a path field when it uses an earlier
   recursive field and its type is `Path F x y`: F the family at its own
   parameters, x and y earlier recursive fields (S4 item 14). An other
   field over a recursive field has no fold case type. */
static void path_field(const Machine *m, uint32_t fam, FieldInfo *fields, uint32_t earlier) {
  FieldInfo *f = &fields[earlier];
  const Core *t = f->type;
  int side;
  f->path = f->over_recursive && t->kind == CORE_PATHP && own_family(m, fam, t->b, earlier + 1u);
  for (side = 0; side < 2; side++) {
    const Core *e = f->path ? t->args[side] : NULL;
    f->path = f->path && e != NULL && e->kind == CORE_VAR && e->index < earlier && fields[earlier - 1u - e->index].recursive;
    f->ends[side] = f->path ? earlier - 1u - e->index : 0u;
  }
}

/* A term that the checker makes, at the place of CTOR. */
static Term *made_term(Checker *c, const Ctor *ctor, TermKind kind, const char *name, const Term *left, const Term *right) {
  Term *t = arena_alloc(c->m->arena, sizeof *t);
  if (t == NULL) {
    FAIL(c, "OOM", "out of memory");
    return NULL;
  }
  t->kind = kind;
  t->line = ctor->line;
  t->col = ctor->col;
  t->name = name;
  t->nat = 0;
  t->left = left;
  t->right = right;
  return t;
}

/* COVER \/ (~NAME \/ NAME), or ~NAME \/ NAME when COVER is NULL. */
static const Term *cover_term(Checker *c, const Ctor *ctor, const Term *cover, const char *name) {
  const Term *v = made_term(c, ctor, TERM_VAR, name, NULL, NULL);
  const Term *neg = v == NULL ? NULL : made_term(c, ctor, TERM_INEG, NULL, v, NULL);
  const Term *ends = neg == NULL ? NULL : made_term(c, ctor, TERM_IJOIN, NULL, neg, v);
  return cover == NULL || ends == NULL ? ends : made_term(c, ctor, TERM_IJOIN, NULL, cover, ends);
}

/* The boundary of CTOR under its fields: the first POINTS fields of CTOR
   (with FIELDS their checked types) and then its interval fields are
   locals. The family parameters are already locals (check_family). */
static const Core *boundary_core(Checker *c, uint32_t fam, const Ctor *ctor, const FieldInfo *fields, size_t points) {
  Machine *m = c->m;
  uint32_t p = m->families[fam].param_count;
  const Value **params = arena_alloc(m->arena, ((size_t)p + 1u) * sizeof *params);
  Term *tube = arena_alloc(m->arena, sizeof *tube);
  const Term *cover = NULL;
  const Core *phi;
  const Core *a;
  Branch *bs;
  uint32_t n = 0;
  uint32_t i;
  size_t j;
  if (params == NULL || tube == NULL)
    return nul(FAIL(c, "OOM", "out of memory"));
  for (i = 0; i < p; i++)
    params[i] = val_var(m, c->level - p + i);
  /* Field j is over the earlier fields: evaluate it after they are pushed. */
  for (j = 0; j < points; j++)
    if (!push(c, fields[j].name, here(c, fields[j].type)))
      return NULL;
  for (j = points; j < ctor->field_count; j++) {
    cover = cover_term(c, ctor, cover, ctor->fields[j].name);
    if (cover == NULL || !push_interval(c, ctor->fields[j].name))
      return NULL;
  }
  phi = check(c, cover, interval_type(c));
  a = quote_value(m, c->level, val_op(m, VAL_OP, OP_FAMILY, fam, 0, params, p));
  if (phi == NULL || a == NULL)
    return NULL;
  tube->kind = TERM_LAM;
  tube->line = ctor->line;
  tube->col = ctor->col;
  tube->name = NULL;
  tube->nat = 0;
  tube->left = NULL;
  tube->right = ctor->boundary;
  bs = system_branches(c, tube, here(c, phi), &n);
  if (bs == NULL)
    return NULL;
  for (i = 0; i < n; i++) {
    bs[i].body = hcomp_branch(c, NULL, a, &bs[i]);
    if (bs[i].body == NULL)
      return NULL;
  }
  if (!system_overlaps(c, "_", bs, n))
    return NULL;
  return system_core(c, "_", bs, n);
}

/* The boundary `[ k1 -> t1, ... ]` of a path constructor (S4 R1 (a)),
   checked as the tube of an hcomp at the family: each branch has the
   family type at the parameters, the keys cover ~i \/ i (and ~j \/ j)
   (SYSTEM_COVER) and two branches agree where both keys hold
   (SYSTEM_OVERLAP). The core is OP_SYSTEM, each body under a path binder
   that no term can use, as an hcomp tube. The context comes back as it
   was. */
static const Core *check_boundary(Checker *c, uint32_t fam, const Ctor *ctor, const FieldInfo *fields, size_t points) {
  const Scope *scope = c->scope;
  const Env *env = c->env;
  uint32_t level = c->level;
  const Core *sys = boundary_core(c, fam, ctor, fields, points);
  c->scope = scope;
  c->env = env;
  c->level = level;
  return sys;
}

/* A constructor takes point fields, then (a path constructor) 1 or 2
   interval fields `(i : I)` and a boundary. */
static int check_ctor(Checker *c, uint32_t fam, const Ctor *ctor) {
  Machine *m = c->m;
  FieldInfo *fields = arena_alloc(m->arena, (ctor->field_count + 1u) * sizeof *fields);
  CtorInfo *ci = &m->ctors[m->ctor_count];
  const Scope *scope = c->scope;
  const Env *env = c->env;
  uint32_t level = c->level;
  const Core *boundary = NULL;
  size_t points = 0;
  size_t j;
  if (fields == NULL)
    return FAIL(c, "OOM", "out of memory");
  if (!name_free(c, ctor->name))
    return 0;
  for (j = 0; j < ctor->field_count; j++) {
    size_t earlier;
    const char *name = ctor->fields[j].name;
    int interval = is_interval(c, ctor->fields[j].type);
    if (strcmp(name, ctor->name) == 0)
      return FAIL(c, "REFUSE_NAME", "%s is the constructor name (rule R4)", name);
    for (earlier = 0; earlier < j; earlier++)
      if (strcmp(name, ctor->fields[earlier].name) == 0)
        return FAIL(c, "REFUSE_NAME", "%s is already a field of this constructor (rule R4)", name);
    if (m->families[fam].ctor_count > 1u && strcmp(name, "tag") == 0)
      return FAIL(c, "DOMAIN_FAMILY", "tag is reserved for the JSON constructor tag in a family with multiple constructors");
    if (!interval && points < j)
      return FAIL(c, "HIT_BOUNDARY", "the point field %s of %s must come before the interval fields", name, ctor->name);
    if (interval && !name_free(c, name))
      return 0;
    if (!interval && !check_field(c, fam, &ctor->fields[j], &fields[j], (uint32_t)points))
      return 0;
    if (!interval)
      path_field(m, fam, fields, (uint32_t)points);
    /* A later field type can use this field (S4 item 13). */
    if (!interval && !push(c, fields[j].name, here(c, fields[j].type)))
      return 0;
    points += !interval;
  }
  c->scope = scope;
  c->env = env;
  c->level = level;
  if (ctor->field_count - points > 2u)
    return FAIL(c, "HIT_BOUNDARY", "%s has %u interval fields; a path constructor has 1 or 2", ctor->name, (unsigned)(ctor->field_count - points));
  if (points < ctor->field_count && ctor->boundary == NULL)
    return FAIL(c, "HIT_BOUNDARY", "the path constructor %s needs a boundary [ ~i -> t0, i -> t1 ] after its interval fields", ctor->name);
  if (points == ctor->field_count && ctor->boundary != NULL)
    return FAIL(c, "HIT_BOUNDARY", "%s has a boundary but no interval field; only a path constructor has a boundary", ctor->name);
  if (ctor->boundary != NULL) {
    boundary = check_boundary(c, fam, ctor, fields, points);
    if (boundary == NULL)
      return 0;
  }
  ci->name = ctor->name;
  ci->family = fam;
  ci->fields = fields;
  ci->field_count = (uint32_t)points;
  ci->interval_count = (uint32_t)(ctor->field_count - points);
  ci->boundary = boundary;
  m->ctor_count++;
  return 1;
}

/* A family is in scope in its own fields, so a field can be recursive. */
static int check_family(Checker *c, const Decl *d) {
  Machine *m = c->m;
  uint32_t fam = m->family_count;
  FamilyInfo *f = &m->families[fam];
  const Core **params = arena_alloc(m->arena, (d->param_count + 1u) * sizeof *params);
  size_t i;
  if (params == NULL)
    return FAIL(c, "OOM", "out of memory");
  for (i = 0; i < d->param_count; i++) {
    uint64_t level = 0;
    params[i] = check_type(c, d->params[i].type, &level);
    if (params[i] == NULL || !push(c, d->params[i].name, here(c, params[i])))
      return 0;
  }
  f->name = d->name;
  f->param_types = params;
  f->param_count = (uint32_t)d->param_count;
  f->first_ctor = m->ctor_count;
  f->ctor_count = (uint32_t)d->ctor_count;
  m->family_count++;
  c->self = fam;
  for (i = 0; i < d->ctor_count; i++) {
    if (!check_ctor(c, fam, &d->ctors[i]))
      return 0;
  }
  return 1;
}

static int check_def(Checker *c, const Decl *d) {
  Machine *m = c->m;
  uint64_t level = 0;
  const Core *t = check_type(c, d->type, &level);
  const Value *tv = here(c, t);
  const Core *body = check(c, d->body, tv);
  DefInfo *info = &m->defs[m->def_count];
  if (body == NULL)
    return 0;
  info->name = d->name;
  info->origin = d->origin;
  info->body = body;
  info->type = tv;
  info->value = NULL;
  m->def_count++;
  return 1;
}

static int check_decl(Checker *c, const Decl *d) {
  if (d->kind == DECL_FAMILY && d->origin == ORIGIN_PROGRAM)
    return FAIL(c, "REFUSE_DATA", "a program cannot declare a family (rule R1); only domain/domain.lang can");
  if (d->kind == DECL_AXIOM)
    return FAIL(c, "REFUSE_AXIOM", "an axiom has no proof (rule R2)");
  if (d->is_rec)
    return FAIL(c, "REFUSE_REC", "def rec is refused (rule R3); use fold or unfold");
  if (!name_free(c, d->name))
    return 0;
  return d->kind == DECL_FAMILY ? check_family(c, d) : check_def(c, d);
}

int check_program(Arena *arena, const DeclList *decls, Machine *m, Diag *diag) {
  Checker c;
  size_t ctors = 0;
  size_t i;
  memset(m, 0, sizeof *m);
  m->arena = arena;
  m->diag = diag;
  m->fuel = EVAL_FUEL_STEPS;
  for (i = 0; i < decls->count; i++)
    ctors += decls->items[i].ctor_count;
  m->defs = arena_alloc(arena, (decls->count + 1u) * sizeof *m->defs);
  m->families = arena_alloc(arena, (decls->count + 1u) * sizeof *m->families);
  m->ctors = arena_alloc(arena, (ctors + 1u) * sizeof *m->ctors);
  if (m->defs == NULL || m->families == NULL || m->ctors == NULL)
    return diag_fail(diag, "OOM", NULL, "out of memory");
  m->def_cap = (uint32_t)decls->count;
  m->family_cap = (uint32_t)decls->count;
  m->ctor_cap = (uint32_t)ctors;
  memset(&c, 0, sizeof c);
  c.m = m;
  c.decls = decls;
  for (i = 0; i < decls->count; i++) {
    c.decl_index = i;
    c.scope = NULL;
    c.env = NULL;
    c.level = 0;
    c.depth = 0;
    c.self = NO_FAMILY;
    m->def = decls->items[i].name;
    if (!check_decl(&c, &decls->items[i]))
      return 0;
  }
  m->def = NULL;
  return 1;
}

/* The literal bound of a Fin type, or 0 (Nat, Fin 0 and a Fin at a variable
   bound all give 0). */
static uint64_t fin_literal(const Value *t) {
  const Value *n = t != NULL && val_is(t, OP_FIN) ? t->args[0] : NULL;
  return n != NULL && n->kind == VAL_NAT ? n->nat : 0u;
}

/* A first-order type: Nat (BOUND 0), Flag (BOUND 0, *FLAG 1), or Fin n at a
   literal n that is not 0 (BOUND n). */
static int first_order(const Value *t, uint64_t *bound, int *flag) {
  *bound = fin_literal(t);
  *flag = t != NULL && val_is(t, OP_FLAG);
  return (t != NULL && val_is(t, OP_NAT)) || *flag || *bound != 0u;
}

int entry_of(Machine *m, const Value *type, Entry *out) {
  const Value *t = type;
  uint64_t bound = 0;
  int flag = 0;
  memset(out, 0, sizeof *out);
  while (t != NULL && t->kind == VAL_PI && out->param_count < ENTRY_PARAMS_MAX) {
    if (!first_order(t->dom, &bound, &flag))
      return 0;
    out->param_bound[out->param_count] = bound;
    out->param_flag[out->param_count] = flag;
    t = closure_apply(m, t, val_var(m, out->param_count));
    out->param_count++;
  }
  if (!first_order(t, &bound, &flag))
    return 0;
  out->result_flag = flag;
  return 1;
}

static int parse_u64(const char *s, uint64_t *out) {
  uint64_t n = 0;
  size_t i;
  if (s[0] == '\0')
    return 0;
  for (i = 0; s[i] != '\0'; i++) {
    unsigned d = (unsigned)(unsigned char)s[i] - (unsigned)'0';
    if (d > 9u || n > (UINT64_MAX - d) / 10u)
      return 0;
    n = n * 10u + d;
  }
  *out = n;
  return 1;
}

/* An entry argument: a Nat in decimal, a Flag as 0 or 1 (IS_FLAG), or a Fin n
   as a decimal below n (BOUND is n, or 0 for a Nat or a Flag). */
static const Value *entry_arg(Machine *m, const char *text, uint64_t bound, int is_flag) {
  uint64_t n = 0;
  if (is_flag && parse_u64(text, &n) && n <= 1u)
    return val_make(m, n == 1u ? OP_FLAG_YES : OP_FLAG_NO, 0, 0, NULL, NULL, NULL);
  if (is_flag)
    diag_fail(m->diag, "EVAL_ARGS", m->def, "'%s' is not a Flag (0 or 1)", text);
  else if (parse_u64(text, &n) && (bound == 0u || n < bound))
    return val_nat(m, n);
  else if (bound == 0u)
    diag_fail(m->diag, "EVAL_ARGS", m->def, "'%s' is not a Nat (0 to 2^64-1)", text);
  else
    diag_fail(m->diag, "EVAL_ARGS", m->def, "'%s' is not a Fin %llu (0 to %llu)", text, (unsigned long long)bound, (unsigned long long)(bound - 1u));
  return NULL;
}

static int print_result(Machine *m, const char *name, const Value *v, FILE *out, FILE *err) {
  switch (v->kind) {
  case VAL_NAT:
    fprintf(out, "%llu\n", (unsigned long long)v->nat);
    return 0;
  case VAL_TRAP:
    fputs("trap\n", out);
    fprintf(err, "langc: EVAL_OVERFLOW: %s: a Nat operation overflowed past 2^64-1\n", name);
    return 0;
  case VAL_OP:
    if (val_is(v, OP_FLAG_YES) || val_is(v, OP_FLAG_NO)) {
      fprintf(out, "%d\n", val_is(v, OP_FLAG_YES));
      return 0;
    }
    break;
  case VAL_UNIV:
  case VAL_LAM:
  case VAL_PI:
  case VAL_SIGMA:
  case VAL_VAR:
  case VAL_APP:
  case VAL_STUCK:
  case VAL_PLAM:
  case VAL_PATHP:
  case VAL_IVAL:
  case VAL_PAPP:
    break;
  }
  diag_fail(m->diag, "EVAL_ENTRY", name, "the entry did not compute a number");
  return 1;
}

int eval_command(Machine *m, const char *name, char *const *args, int arg_count, FILE *out, FILE *err) {
  uint32_t d = find_def(m, name);
  Entry entry;
  const Value *v;
  char *buf;
  uint32_t i;
  m->def = name;
  if (d == NO_DEF) {
    diag_fail(m->diag, "EVAL_ENTRY", name, "no definition has this name");
    return 1;
  }
  v = def_value(m, d);
  if (v == NULL)
    return 1;
  if (!entry_of(m, m->defs[d].type, &entry) && arg_count != 0) {
    diag_fail(m->diag, "EVAL_ARGS", name, "the definition is not an entry, so it takes no arguments");
    return 2;
  }
  if (!entry_of(m, m->defs[d].type, &entry)) {
    buf = arena_alloc(m->arena, PRINT_MAX);
    if (buf == NULL)
      return diag_fail(m->diag, "OOM", name, "out of memory") + 1;
    if (!value_print(m, NULL, 0, v, buf, PRINT_MAX)) {
      diag_fail(m->diag, "EVAL_PRINT", name, "the normal form cannot be printed as source syntax");
      return 1;
    }
    fprintf(out, "%s\n", buf);
    return 0;
  }
  if ((uint32_t)arg_count != entry.param_count) {
    diag_fail(m->diag, "EVAL_ARGS", name, "the entry takes %u arguments, found %d", entry.param_count, arg_count);
    return 2;
  }
  for (i = 0; i < entry.param_count && v != NULL; i++) {
    const Value *arg = entry_arg(m, args[i], entry.param_bound[i], entry.param_flag[i]);
    v = arg == NULL ? NULL : apply_value(m, v, arg);
  }
  if (v == NULL)
    return strcmp(m->diag->code != NULL ? m->diag->code : "", "EVAL_ARGS") == 0 ? 2 : 1;
  return print_result(m, name, v, out, err);
}
