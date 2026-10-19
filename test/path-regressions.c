#include <stdio.h>
#include <string.h>

#include "front/front.h"
#include "front/check.h"
#include "json.h"

static int cases_run = 0;

static int checked(Arena *arena, Machine *m, Diag *diag, const char *source) {
  DeclList decls;
  diag_init(diag);
  return front_load(arena, "path-regression", source, strlen(source), &decls, diag)
      && check_program(arena, &decls, m, diag);
}

static int accepts(const char *label, const char *source) {
  cases_run++;
  Arena arena;
  Machine m;
  Diag diag;
  int ok;
  arena_init(&arena, (size_t)128 << 20);
  ok = checked(&arena, &m, &diag, source);
  if (!ok) {
    fprintf(stderr, "FAIL %s\n", label);
    diag_print(&diag, stderr);
  }
  arena_release(&arena);
  return ok;
}

/* SOURCE after the extra domain text DOMAIN, which can declare a family (a
   program cannot: REFUSE_DATA). */
static int domain_checked(Arena *arena, Machine *m, Diag *diag, const char *domain, const char *source) {
  DeclList decls;
  diag_init(diag);
  return front_load_domain(arena, "path-regression", domain, source, strlen(source), &decls, diag)
      && check_program(arena, &decls, m, diag);
}

static int domain_accepts(const char *label, const char *domain, const char *source) {
  cases_run++;
  Arena arena;
  Machine m;
  Diag diag;
  int ok;
  arena_init(&arena, (size_t)128 << 20);
  ok = domain_checked(&arena, &m, &diag, domain, source);
  if (!ok) {
    fprintf(stderr, "FAIL %s\n", label);
    diag_print(&diag, stderr);
  }
  arena_release(&arena);
  return ok;
}

/* 1 when DOMAIN and SOURCE fail and the first diagnostic has CODE. */
static int domain_refuses(const char *label, const char *domain, const char *source, const char *code) {
  cases_run++;
  Arena arena;
  Machine m;
  Diag diag;
  int ok;
  arena_init(&arena, (size_t)128 << 20);
  ok = !domain_checked(&arena, &m, &diag, domain, source) && diag.set && strcmp(diag.code, code) == 0;
  if (!ok) {
    fprintf(stderr, "FAIL %s: expected %s\n", label, code);
    if (diag.set)
      diag_print(&diag, stderr);
  }
  arena_release(&arena);
  return ok;
}

static int system_conversion(void) {
  cases_run++;
  Arena arena;
  Machine m;
  Diag diag;
  Value lines[2] = {{0}, {0}};
  Value negative;
  IConj conjunction;
  uint32_t negative_lit = 1;
  const Value *points[2];
  const Value *args[6];
  const Value *i;
  const Value *ni;
  const Value *original;
  const Value *other;
  int ok;
  uint32_t k;
  arena_init(&arena, (size_t)128 << 20);
  ok = checked(&arena, &m, &diag, "def seed : Nat := 0\n");
  if (ok) {
    for (k = 0; k < 2u; k++) {
      points[k] = val_nat(&m, k);
      lines[k].kind = VAL_PLAM;
      lines[k].tag = CLOS_CONST;
      lines[k].args = &points[k];
      lines[k].argc = 1;
    }
    i = val_ivar(&m, 0);
    negative = *i;
    conjunction.lits = &negative_lit;
    conjunction.count = 1;
    negative.conj = &conjunction;
    ni = &negative;
    args[0] = i; args[1] = &lines[0];
    args[2] = ni; args[3] = &lines[1];
    original = val_op(&m, VAL_OP, OP_SYSTEM, 0, 0, args, 4);
    args[0] = ni; args[1] = &lines[1];
    args[2] = i; args[3] = &lines[0];
    other = val_op(&m, VAL_OP, OP_SYSTEM, 0, 0, args, 4);
    ok = conv_values(&m, 2, original, other);
    args[1] = &lines[0]; args[3] = &lines[1];
    other = val_op(&m, VAL_OP, OP_SYSTEM, 0, 0, args, 4);
    ok = ok && !conv_values(&m, 2, original, other);
    args[0] = i; args[1] = &lines[0];
    other = val_op(&m, VAL_OP, OP_SYSTEM, 0, 0, args, 2);
    ok = ok && !conv_values(&m, 2, original, other);
    args[2] = ni; args[3] = &lines[1];
    args[4] = face_meet(&m, i, val_ivar(&m, 1)); args[5] = &lines[0];
    other = val_op(&m, VAL_OP, OP_SYSTEM, 0, 0, args, 6);
    ok = ok && conv_values(&m, 2, original, other) && !diag.set;
  }
  if (!ok) {
    fprintf(stderr, "FAIL system conversion: order, values, coverage or refinement\n");
    if (diag.set) diag_print(&diag, stderr);
  }
  arena_release(&arena);
  return ok;
}

static int domain_roundtrip(const char *label, const char *domain, const char *source) {
  cases_run++;
  Arena arena;
  Machine m, replay;
  Diag diag;
  char type[8192], value[8192], program[32768];
  int ok;
  uint32_t index;
  arena_init(&arena, (size_t)128 << 20);
  ok = domain_checked(&arena, &m, &diag, domain, source);
  index = ok ? m.def_count - 1u : 0u;
  if (ok)
    ok = value_print(&m, NULL, 0, m.defs[index].type, type, sizeof type)
      && value_print(&m, NULL, 0, def_value(&m, index), value, sizeof value);
  if (ok) {
    snprintf(program, sizeof program, "def replay : %s := %s\n", type, value);
    ok = domain_checked(&arena, &replay, &diag, domain, program);
    if (!ok) fprintf(stderr, "%s\n", program);
  }
  if (ok)
    ok = conv_values(&m, 0, m.defs[index].type, replay.defs[replay.def_count - 1u].type)
      && conv_values(&m, 0, def_value(&m, index), def_value(&replay, replay.def_count - 1u));
  if (!ok) {
    fprintf(stderr, "FAIL %s\n", label);
    diag_print(&diag, stderr);
  }
  arena_release(&arena);
  return ok;
}

static int roundtrip(const char *label, const char *source) {
  return domain_roundtrip(label, "", source);
}

static int domain_json_contains(const char *label, const char *domain, const char *source, const char *expected) {
  cases_run++;
  Arena arena;
  Machine m;
  Diag diag;
  const char *text = NULL;
  size_t len = 0;
  int ok;
  arena_init(&arena, (size_t)128 << 20);
  ok = domain_checked(&arena, &m, &diag, domain, source)
    && json_document(&m, &text, &len)
    && strstr(text, expected) != NULL;
  if (!ok) {
    fprintf(stderr, "FAIL %s: expected JSON containing %s\n", label, expected);
    diag_print(&diag, stderr);
  }
  arena_release(&arena);
  return ok;
}

static int json_contains(const char *label, const char *source, const char *expected) {
  return domain_json_contains(label, "", source, expected);
}

/* Same-name instances of the same type must retain distinct path data. */
static int distinct_path_json(const char *label, const char *first, const char *second) {
  Arena arena;
  Machine a, b;
  Diag diag;
  const char *left = NULL;
  const char *right = NULL;
  size_t left_len = 0, right_len = 0;
  uint32_t index;
  int ok;
  cases_run++;
  arena_init(&arena, (size_t)128 << 20);
  ok = checked(&arena, &a, &diag, first) && checked(&arena, &b, &diag, second);
  index = ok ? a.def_count - 1u : 0u;
  if (ok)
    ok = a.def_count == b.def_count
      && conv_values(&a, 0, a.defs[index].type, b.defs[index].type)
      && !conv_values(&a, 0, def_value(&a, index), def_value(&b, index)) && !diag.set
      && json_document(&a, &left, &left_len) && json_document(&b, &right, &right_len)
      && (left_len != right_len || strcmp(left, right) != 0);
  if (!ok) {
    fprintf(stderr, "FAIL %s: distinct paths need distinct JSON\n", label);
    if (diag.set) diag_print(&diag, stderr);
  }
  arena_release(&arena);
  return ok;
}

int main(void) {
  int failed = 0;
  failed += !system_conversion();
  failed += !accepts("Glue composition with reordered faces",
    "def G : Path (Type 0) (Fin 3) (Fin 3) := fun (j : I) => Glue (Fin 3) (~j \\/ j) (fun (_ : I) => [ ~j -> pack (Fin 3) (idEquiv (Fin 3)), j -> pack (Fin 3) (idEquiv (Fin 3)) ])\n"
    "def g : PathP (fun (j : I) => G @ j) 0 0 := fun (j : I) => glue (fun (_ : I) => [ ~j \\/ j -> 0 ]) 0\n"
    "def same : PathP (fun (j : I) => Path (G @ j) (hcomp (G @ j) (~j \\/ j) (fun (_ : I) => [ ~j -> g @ j, j -> g @ j ]) (g @ j)) (g @ j)) refl refl := fun (j : I) => refl\n");
  failed += !accepts("Glue composition under a stronger face",
    "def G0 : Path (Type 0) (Fin 3) (Fin 3) := fun (j : I) => Glue (Fin 3) (~j \\/ j) (fun (_ : I) => [ ~j -> pack (Fin 3) (idEquiv (Fin 3)), j -> pack (Fin 3) (idEquiv (Fin 3)) ])\n"
    "def g0 : PathP (fun (j : I) => G0 @ j) 0 0 := fun (j : I) => glue (fun (_ : I) => [ ~j \\/ j -> 0 ]) 0\n"
    "def G1 : Path (Type 0) (Fin 3) (Fin 3) := fun (j : I) => Glue (Fin 3) (~j \\/ j) (fun (_ : I) => [ ~j -> pack (Fin 3) (idEquiv (Fin 3)), j -> pack (Fin 3) (idEquiv (Fin 3)), ~j -> pack (Fin 3) (idEquiv (Fin 3)) ])\n"
    "def g1 : PathP (fun (j : I) => G1 @ j) 0 0 := fun (j : I) => glue (fun (_ : I) => [ ~j \\/ j -> 0 ]) 0\n"
    "def G : Path (Path (Type 0) (Fin 3) (Fin 3)) G0 G1 := fun (i : I) => fun (j : I) => Glue (Fin 3) (~j \\/ j) (fun (_ : I) => [ ~j -> pack (Fin 3) (idEquiv (Fin 3)), j -> pack (Fin 3) (idEquiv (Fin 3)), ~j /\\ i -> pack (Fin 3) (idEquiv (Fin 3)) ])\n"
    "def g : PathP (fun (i : I) => PathP (fun (j : I) => G @ i @ j) 0 0) g0 g1 := fun (i : I) => fun (j : I) => glue (fun (_ : I) => [ ~j \\/ j -> 0 ]) 0\n"
    "def same0 : PathP (fun (j : I) => Path (G0 @ j) (hcomp (G0 @ j) (~j \\/ j) (fun (_ : I) => [ ~j -> g0 @ j, j -> g0 @ j ]) (g0 @ j)) (g0 @ j)) refl refl := fun (j : I) => refl\n"
    "def same1 : PathP (fun (j : I) => Path (G1 @ j) (hcomp (G1 @ j) (~j \\/ j) (fun (_ : I) => [ ~j -> g1 @ j, j -> g1 @ j ]) (g1 @ j)) (g1 @ j)) refl refl := fun (j : I) => refl\n"
    "def same : PathP (fun (i : I) => PathP (fun (j : I) => Path (G @ i @ j) (hcomp (G @ i @ j) (~j \\/ j) (fun (_ : I) => [ ~j -> g @ i @ j, j -> g @ i @ j ]) (g @ i @ j)) (g @ i @ j)) refl refl) same0 same1 := fun (i : I) => fun (j : I) => refl\n");
  failed += !roundtrip("PathP binder capture",
    "def original : Sigma (PathP : Nat) (Path Nat PathP PathP) := pack 0 refl\n");
  failed += !roundtrip("anonymous funExt argument",
    "def original : (h : (_ : Nat) -> Path Nat 0 0) -> Path (Nat -> Nat) (fun n => 0) (fun n => 0) := fun h => funExt h\n");
  failed += !roundtrip("application of a path of functions",
    "def original : (p : Path (Nat -> Nat) (fun n => n) (fun n => n)) -> Path Nat 0 0 := fun p => fun (i : I) => (p @ i) 0\n");
  failed += !roundtrip("dependent congruence",
    "def original : (p : Path Nat 0 0) -> PathP (fun (i : I) => Lot (p @ i)) (makeLot 0 7) (makeLot 0 7) := fun p => cong (fun (n : Nat) => makeLot n 7) p\n");
  failed += !roundtrip("dependent symmetry",
    "def original : (A : Path (Type 0) Nat Nat) -> (p : PathP (fun (j : I) => A @ j) 0 1) -> PathP (fun (i : I) => A @ ~i) 1 0 := fun A p => sym p\n");
  failed += !roundtrip("dependent function extensionality",
    "def original : (A : Path (Type 0) Nat Nat) -> (h : (n : Nat) -> PathP (fun (j : I) => A @ j) n n) -> PathP (fun (i : I) => Nat -> A @ i) (fun n => n) (fun n => n) := fun A h => funExt h\n");
  failed += !roundtrip("stuck transport fill",
    "def original : PathP (fun (i : I) => (p : Path (Type 0) Nat Nat) -> p @ i) (fun (p : Path (Type 0) Nat Nat) => 0) (fun (p : Path (Type 0) Nat Nat) => transport Nat Nat p 0) := fun (i : I) => fun (p : Path (Type 0) Nat Nat) => transp (fun (j : I) => p @ (i /\\ j)) (~i) 0\n");
  failed += !roundtrip("stuck hcomp with two keys",
    "def original : (n : Nat) -> (p : Path Nat n n) -> Path Nat n n := fun n p => fun (j : I) => hcomp Nat (~j \\/ j) (fun (i : I) => [ ~j -> n, j -> p @ i ]) (p @ j)\n");
  failed += !roundtrip("stuck transport in a Path line",
    "def original : (A : Type 0) -> (x : A) -> (y : A) -> (r : Path A x y) -> (p : Path A x x) -> Path A y y := fun A x y r p => transp (fun (i : I) => Path A (r @ i) (r @ i)) i0 p\n");
  failed += !roundtrip("dead constructor branch",
    "def original : (x : Prod Nat Nat) -> (n : Nat) -> (p : Path Nat n n) -> Path (Prod Nat Nat) (pair n 0) (pair n 0) := fun x n p => fun (j : I) => hcomp (Prod Nat Nat) (~j \\/ j) (fun (i : I) => [ ~j -> pair n 0, j -> pair (p @ i) 0, i0 -> x ]) (pair n 0)\n");
  failed += !roundtrip("dependent Sigma composition",
    "def original : (n : Nat) -> (p : Path Nat n n) -> Path (Sigma (x : Nat) (Path Nat x x)) (pack n refl) (pack n refl) := fun n p => fun (j : I) => hcomp (Sigma (x : Nat) (Path Nat x x)) (~j \\/ j) (fun (i : I) => [ ~j -> pack n refl, j -> pack (p @ i) refl ]) (pack n refl)\n");
  failed += !roundtrip("universe function composition",
    "def original : (p : Path (Nat -> Type 0) (fun n => Nat) (fun n => Nat)) -> Path (Nat -> Type 0) (fun n => Nat) (fun n => Nat) := fun p => fun (j : I) => hcomp (Nat -> Type 0) (~j \\/ j) (fun (i : I) => [ ~j -> (fun n => Nat), j -> p @ i ]) (fun n => Nat)\n");
  failed += !roundtrip("universe Sigma composition",
    "def original : (p : Path (Sigma (x : Nat) (Type 0)) (pack 0 Nat) (pack 0 Nat)) -> Path (Sigma (x : Nat) (Type 0)) (pack 0 Nat) (pack 0 Nat) := fun p => fun (j : I) => hcomp (Sigma (x : Nat) (Type 0)) (~j \\/ j) (fun (i : I) => [ ~j -> pack 0 Nat, j -> p @ i ]) (pack 0 Nat)\n");
  failed += !roundtrip("universe product composition",
    "def original : (p : Path (Prod Nat (Type 0)) (pair 0 Nat) (pair 0 Nat)) -> Path (Prod Nat (Type 0)) (pair 0 Nat) (pair 0 Nat) := fun p => fun (j : I) => hcomp (Prod Nat (Type 0)) (~j \\/ j) (fun (i : I) => [ ~j -> pair 0 Nat, j -> pair 0 Nat ]) (pair 0 Nat)\n");
  failed += !roundtrip("universe path composition",
    "def original : (p : Path (Path (Type 0) Nat Nat) refl refl) -> Path (Path (Type 0) Nat Nat) refl refl := fun p => fun (j : I) => hcomp (Path (Type 0) Nat Nat) (~j \\/ j) (fun (i : I) => [ ~j -> refl, j -> p @ i ]) refl\n");
  failed += !roundtrip("transport of a universe path",
    "def original : (p : Path (Type 0) Nat Nat) -> Path (Type 0) Nat Nat := fun p => transp (fun (_ : I) => Path (Type 0) Nat Nat) i0 p\n");
  failed += !roundtrip("face action on a local type",
    "def original : (P : Path (Type 0) Nat Nat) -> PathP (fun (j : I) => (n : P @ j) -> P @ j) (fun (n : Nat) => n) (fun (n : Nat) => n) := fun P => fun (j : I) => fun (n : P @ j) => hcomp (P @ j) (~j \\/ j) (fun (i : I) => [ ~j -> n, j -> n ]) n\n");
  failed += !roundtrip("a branch on a disjunctive face",
    "def original : (p : Path Nat 0 1) -> PathP (fun (j : I) => Path Nat (p @ j) (p @ j)) refl refl := fun p => fun (j : I) => hcomp (Path Nat (p @ j) (p @ j)) (~j \\/ j) (fun (i : I) => [ ~j \\/ j -> refl ]) refl\n");
  failed += !roundtrip("stuck Fin addition",
    "def original : (r : Fin 11) -> Fin 11 := fun (r : Fin 11) => finAdd 11 r 3\n");
  /* S5a item 2: a finAll def prints as finCase and round-trips. */
  failed += !roundtrip("finite case function printing",
    "def original : (k : Fin 2) -> Path (Fin 2) (finAdd 2 k 0) k := fun k => refl\n");
  failed += !roundtrip("finite case path printing",
    "def p : (k : Fin 2) -> Path (Fin 2) (finAdd 2 k 0) k := fun k => refl\n"
    "def original : Path (Fin 2 -> Fin 2) (fun k => finAdd 2 k 0) (fun k => k) := funExt p\n");
  failed += !roundtrip("finite case beyond the ordinary spine limit",
    "def original : (k : Fin 63) -> Path (Fin 63) (finAdd 63 k 0) k := fun k => refl\n");
  failed += !roundtrip("finite function case in an application",
    "def choose : Fin 2 -> Nat -> Nat := fun k => finCase 2 k (fun n => natAdd n 1) (fun n => natAdd n 2)\n"
    "def original : Fin 2 -> Nat -> Nat := fun k n => choose k n\n");
  failed += !accepts("finite case refines dependent local types",
    "def f : (k : Fin 2) -> (x : finCase 2 k Nat (Fin 3)) -> Nat := fun k x => finCase 2 k x (finVal 3 x)\n"
    "def zero : Path Nat (f 0 7) 7 := refl\n"
    "def one : Path Nat (f 1 2) 2 := refl\n");
  /* S5a item 5: a finCase at a closed scrutinee checks at the top level
     (the empty env is not an allocation failure). */
  failed += !accepts("finite case at a closed literal scrutinee",
    "def a : Path Nat (finCase 2 1 0 1) 1 := refl\n"
    "def b : Nat := finCase 2 (finEq 3 1 1) 0 1\n");
  failed += !roundtrip("Glue type at a neutral face",
    "def original : (A : Type 0) -> (e : Equiv A A) -> (f : Equiv A A) -> Path (Type 0) A A := fun A e f => fun (i : I) => Glue A (~i \\/ i) (fun (_ : I) => [ ~i -> pack A e, i -> pack A f ])\n");
  failed += !roundtrip("stuck unglue",
    "def original : (A : Type 0) -> (e : Equiv A A) -> (x : A) -> (y : A) -> (g : PathP (fun (i : I) => Glue A (~i \\/ i) (fun (_ : I) => [ ~i -> pack A e, i -> pack A e ])) x y) -> Path A (witness e x) (witness e y) := fun A e x y g => fun (i : I) => unglue (g @ i)\n");
  failed += !roundtrip("univalence at a Glue endpoint",
    "def G : Type 0 := Glue (Fin 3) i0 (fun (_ : I) => [ ])\n"
    "def u : G := glue (fun (_ : I) => [ ]) 0\n"
    "def original : G := transport G G (ua G G (idEquiv G)) u\n");
  failed += !roundtrip("unglue at a nested Glue face",
    "def G : Type 0 := Glue (Fin 3) i0 (fun (_ : I) => [ ])\n"
    "def u : G := glue (fun (_ : I) => [ ]) 0\n"
    "def p : PathP (fun (i : I) => Glue G (~i \\/ i) (fun (_ : I) => [ ~i -> pack G (idEquiv G), i -> pack G (idEquiv G) ])) u u := fun (i : I) => glue (fun (_ : I) => [ ~i -> u, i -> u ]) u\n"
    "def original : PathP (fun (i : I) => Path G (unglue (p @ i)) u) refl refl := fun (i : I) => refl\n");
  failed += !json_contains("transport table with function points",
    "def singletonFunPath : (h : Fin 1 -> Fin 1) -> (x : Fin 1) -> Path (Fin 1) (h 0) (h x) := fun h x => refl\n"
    "def toFun : Fin 1 -> (Fin 1 -> Fin 1) := fun x => fun (_ : Fin 1) => x\n"
    "def fromFun : (Fin 1 -> Fin 1) -> Fin 1 := fun h => h 0\n"
    "def toFromFun : (h : Fin 1 -> Fin 1) -> Path (Fin 1 -> Fin 1) (toFun (fromFun h)) h := fun h => fun (i : I) => fun x => singletonFunPath h x @ i\n"
    "def fromToFun : (x : Fin 1) -> Path (Fin 1) (fromFun (toFun x)) x := fun x => refl\n"
    "def finToFun : Path (Type 0) (Fin 1) (Fin 1 -> Fin 1) := ua (Fin 1) (Fin 1 -> Fin 1) (isoToEquiv (Fin 1) (Fin 1 -> Fin 1) toFun fromFun toFromFun fromToFun)\n",
    "\"transport\":[\"fun x => 0\"]");
  failed += !json_contains("stuck transport table uses path syntax",
    "def neutralLoop : Path (Type 0) (Fin 1) (Fin 1) := hcomp (Path (Type 0) (Fin 1) (Fin 1)) i0 (fun (_ : I) => [ ]) refl\n",
    "\"term\":\"fun i =>");
  failed += !domain_accepts("path constructor declaration",
    "family S1x := basex | loopx (i : I) [ ~i -> basex, i -> basex ]\n",
    "def p : Path S1x (loopx i0) (loopx i1) := fun (i : I) => loopx i\n");
  failed += !domain_refuses("path constructor boundary of a wrong type",
    "family S1x := basex | loopx (i : I) [ ~i -> 0, i -> basex ]\n",
    "def u : Nat := 0\n", "TYPE_MISMATCH");
  failed += !domain_refuses("path constructor boundary that does not cover",
    "family S1x := basex | loopx (i : I) [ ~i -> basex ]\n",
    "def u : Nat := 0\n", "SYSTEM_COVER");
  failed += !domain_refuses("path constructor with no boundary",
    "family S1x := basex | loopx (i : I)\n",
    "def u : Nat := 0\n", "HIT_BOUNDARY");
  failed += !domain_json_contains("path tree fallback after a recursive field",
    "family Tree := leaf | branch (subtree : Tree) (label : S1) (i : I) [ ~i -> subtree, i -> subtree ]\n",
    "def p : Path Tree leaf leaf := fun (j : I) => branch leaf (loop j) j\n",
    "\"value\":{\"from\":{\"tag\":\"leaf\"},\"to\":{\"tag\":\"leaf\"},\"term\":\"fun j => branch leaf (loop j) j\"}}]}");
  failed += !domain_json_contains("path tree with a recursive field",
    "family Tree := leaf | branch (subtree : Tree) (label : S1) (i : I) [ ~i -> subtree, i -> subtree ]\n",
    "def p : Path Tree leaf leaf := fun (j : I) => branch (branch leaf base j) base j\n",
    "\"value\":{\"path\":\"branch\",\"args\":[{\"path\":\"branch\",\"args\":[{\"tag\":\"leaf\"},\"base\"]},\"base\"]}}]}");
  failed += !roundtrip("formal hcomp at a HIT",
    "def original : Path S1 base base := fun (j : I) => hcomp S1 (~j \\/ j) (fun (i : I) => [ ~j -> base, j -> loop i ]) (loop j)\n");
  failed += !distinct_path_json("formal hcomp retains its tube",
    "def p : Path S1 base base := fun (j : I) => hcomp S1 (~j \\/ j) (fun (i : I) => [ ~j -> base, j -> base ]) (loop j)\n",
    "def p : Path S1 base base := fun (j : I) => hcomp S1 (~j \\/ j) (fun (i : I) => [ ~j -> base, j -> loop i ]) (loop j)\n");
  failed += !roundtrip("path constructor at a neutral interval",
    "def original : PathP (fun (i : I) => Path S1 base (loop i)) refl (fun (j : I) => loop j) := fun (i : I) => fun (j : I) => loop (i /\\ j)\n");
  failed += !roundtrip("Glue endpoint substitution preserves interval connections",
    "def code : S1 -> S1 -> Type 0 := fun (y : S1) => fun (x : S1) =>\n"
    "  fold (Path S1 y y) (ua (Path S1 y y) (Path S1 y y) (idEquiv (Path S1 y y))) x\n"
    "def transportSelf : (y : S1) -> Path S1 y y := fun (y : S1) =>\n"
    "  transp (fun (i : I) => code y (loop i)) i0 (fun (_ : I) => y)\n"
    "def p : PathP (fun (j : I) => Path S1 (loop (j /\\ ~j)) (loop (j /\\ ~j)))\n"
    "  (transportSelf base) (transportSelf base) := fun (j : I) => transportSelf (loop (j /\\ ~j))\n");
  failed += !roundtrip("transport of a formal hcomp at Trunc1 along a parameter line",
    "def original : (A : Type 0) -> (B : Type 0) -> (p : Path (Type 0) A B) -> (a : A) -> (b : A) ->\n"
    "  Path (Trunc1 B) (transp (fun (i : I) => Trunc1 (p @ i)) i0 (trunc1 A a)) (transp (fun (i : I) => Trunc1 (p @ i)) i0 (trunc1 A a)) :=\n"
    "  fun (A : Type 0) => fun (B : Type 0) => fun (p : Path (Type 0) A B) => fun (a : A) => fun (b : A) => fun (j : I) =>\n"
    "  transp (fun (i : I) => Trunc1 (p @ i)) i0 (hcomp (Trunc1 A) (~j \\/ j)\n"
    "    (fun (i : I) => [ ~j -> trunc1 A a, j -> squash1 A (trunc1 A b) (trunc1 A a) i ]) (squash1 A (trunc1 A a) (trunc1 A b) j))\n");
  failed += !domain_roundtrip("transport preserves a parameter-dependent HIT boundary",
    "family ParamHit (a : Trunc1 (Fin 3)) := paramPoint (paramValue : Trunc1 (Fin 3)) |\n"
    "  paramLoop (i : I) [ ~i -> paramPoint a a, i -> paramPoint a a ]\n",
    "def endpoint : ParamHit (trunc1 (Fin 3) 1) :=\n"
    "  transp (fun (i : I) => ParamHit (squash1 (Fin 3) (trunc1 (Fin 3) 0) (trunc1 (Fin 3) 1) i)) i0\n"
    "    (paramPoint (trunc1 (Fin 3) 0) (trunc1 (Fin 3) 0))\n"
    "def original : Path (ParamHit (trunc1 (Fin 3) 1)) endpoint endpoint :=\n"
    "  fun (j : I) => transp (fun (i : I) => ParamHit (squash1 (Fin 3) (trunc1 (Fin 3) 0) (trunc1 (Fin 3) 1) i)) i0\n"
    "    (paramLoop (trunc1 (Fin 3) 0) j)\n");
  /* S4c item 17 (R7 (a)): the transport of a dimension-2 constructor along
     a parameter line stays stuck. Its boundary branches are path
     applications, not recursive fields, so a field rebuild needs a
     boundary correction (R7 (c)). */
  failed += !roundtrip("transport of squash0 along a parameter line stays stuck",
    "def original : (A : Type 0) -> (B : Type 0) -> (p : Path (Type 0) A B) -> (x : Trunc0 A) -> (y : Trunc0 A) ->\n"
    "  (u : Path (Trunc0 A) x y) -> (v : Path (Trunc0 A) x y) ->\n"
    "  Path (Path (Trunc0 B) (transp (fun (k : I) => Trunc0 (p @ k)) i0 x) (transp (fun (k : I) => Trunc0 (p @ k)) i0 y))\n"
    "    (fun (j : I) => transp (fun (k : I) => Trunc0 (p @ k)) i0 (u @ j)) (fun (j : I) => transp (fun (k : I) => Trunc0 (p @ k)) i0 (v @ j)) :=\n"
    "  fun (A : Type 0) => fun (B : Type 0) => fun (p : Path (Type 0) A B) => fun (x : Trunc0 A) => fun (y : Trunc0 A) =>\n"
    "  fun (u : Path (Trunc0 A) x y) => fun (v : Path (Trunc0 A) x y) =>\n"
    "  fun (i : I) => fun (j : I) => transp (fun (k : I) => Trunc0 (p @ k)) i0 (squash0 A x y u v i j)\n");
  failed += !domain_refuses("transport of squash0 is not the field rebuild", "",
    "def same : (A : Type 0) -> (B : Type 0) -> (p : Path (Type 0) A B) -> (x : Trunc0 A) -> (y : Trunc0 A) ->\n"
    "  (u : Path (Trunc0 A) x y) -> (v : Path (Trunc0 A) x y) ->\n"
    "  Path (Path (Path (Trunc0 B) (transp (fun (k : I) => Trunc0 (p @ k)) i0 x) (transp (fun (k : I) => Trunc0 (p @ k)) i0 y))\n"
    "    (fun (j : I) => transp (fun (k : I) => Trunc0 (p @ k)) i0 (u @ j)) (fun (j : I) => transp (fun (k : I) => Trunc0 (p @ k)) i0 (v @ j)))\n"
    "    (fun (i : I) => fun (j : I) => transp (fun (k : I) => Trunc0 (p @ k)) i0 (squash0 A x y u v i j))\n"
    "    (fun (i : I) => fun (j : I) => squash0 B (transp (fun (k : I) => Trunc0 (p @ k)) i0 x) (transp (fun (k : I) => Trunc0 (p @ k)) i0 y)\n"
    "      (fun (k : I) => transp (fun (m : I) => Trunc0 (p @ m)) i0 (u @ k)) (fun (k : I) => transp (fun (m : I) => Trunc0 (p @ m)) i0 (v @ k)) i j) :=\n"
    "  fun (A : Type 0) => fun (B : Type 0) => fun (p : Path (Type 0) A B) => fun (x : Trunc0 A) => fun (y : Trunc0 A) =>\n"
    "  fun (u : Path (Trunc0 A) x y) => fun (v : Path (Trunc0 A) x y) => refl\n", "PATH_UIP");
  failed += !domain_refuses("dimension-2 boundary that does not cover the square",
    "family Sq2 (A : Type 0) := sq2Point (sq2Of : A)\n"
    "  | sq2Square (sq2From : Sq2 A) (sq2To : Sq2 A) (sq2Left : Path (Sq2 A) sq2From sq2To) (sq2Right : Path (Sq2 A) sq2From sq2To) (i : I) (j : I)\n"
    "    [ ~i -> sq2Left @ j, i -> sq2Right @ j, ~j -> sq2From ]\n",
    "def u : Nat := 0\n", "SYSTEM_COVER");
  /* Boundary images must fold path fields just as evaluation does. */
  failed += !domain_roundtrip("fold boundary maps a point constructor path field",
    "family HH := hhBase | hhNode (hhX : HH) (hhY : HH) (hhP : Path HH hhX hhY) | hhLoop (i : I) [ ~i -> hhNode hhBase hhBase refl, i -> hhNode hhBase hhBase refl ]\n",
    "def original : Path (Fin 2) 0 0 := fun (t : I) => fold 0 (fun (x : Fin 2) (y : Fin 2) (p : Path (Fin 2) x y) => p @ t) refl (hhLoop t)\n");
  failed += !domain_roundtrip("fold square boundary preserves captured intervals in path fields",
    "family HH := hhBase | hhNode (hhX : HH) (hhY : HH) (hhP : Path HH hhX hhY)\n"
    "  | hhSquare (hhU : HH) (hhV : Path HH hhU hhU) (i : I) (j : I) [ ~i -> hhNode hhU hhU (fun (k : I) => hhV @ (j /\\ ~j /\\ k /\\ ~k)),\n"
    "      i -> hhNode hhU hhU (fun (k : I) => hhV @ (j /\\ ~j /\\ k /\\ ~k)), ~j -> hhNode hhU hhU refl, j -> hhNode hhU hhU refl ]\n",
    "def original : HH -> HH := fun (z : HH) => fold hhBase (fun (x : HH) (y : HH) (p : Path HH x y) => hhNode x y p) (fun (x : HH) (p : Path HH x x) => fun (a : I) => fun (b : I) => hhSquare x p a b) z\n");
  /* S5b item 7: the refutation of decEqFin is a closure. Its printed form
     must check again at the declared type (J15: Fin 0 and Unit swapped in
     the line fail with TYPE_MISMATCH at i0). */
  failed += !domain_roundtrip("decEqFin inr round-trips", "",
    "def d : Dec (Path (Fin 3) 0 1) := decEqFin 3 0 1\n");
  failed += !roundtrip("decEqNat inr round-trips",
    "def d : Dec (Path Nat 0 1) := decEqNat 0 1\n");
  /* S6a item 1 (R13 (a)): a glue element is neutral for a Nat operation.
     The transport back along the ua line gives finAdd on a glue element;
     the operation stays stuck and rule 6 prints the term (S5c: INTERNAL). */
  failed += !json_contains("Nat operation on a glue element stays stuck",
    "def loopRot1 : Path (Type 0) (Fin 3) (Fin 3) := ua (Fin 3) (Fin 3) (isoToEquiv (Fin 3) (Fin 3) "
    "(fun (r : Fin 3) => finAdd 3 r 1) (fun (r : Fin 3) => finAdd 3 r 2) (fun (k : Fin 3) => refl) (fun (k : Fin 3) => refl))\n"
    "def back : Path (Fin 3) 0 0 := fun (i : I) => transp (fun (j : I) => loopRot1 @ (i /\\ ~j)) i0 "
    "(transp (fun (j : I) => loopRot1 @ (i /\\ j)) (~i) 0)\n",
    "(finAdd 3 (finAdd 3 (glue (fun (_ : I) => [ ~i -> 0, i -> 1 ]) 1) 1) 2)");
  /* Both value operands of finAdd must stay neutral on Glue elements. */
  failed += !json_contains("finAdd right Glue operand stays stuck",
    "def loopRot1 : Path (Type 0) (Fin 3) (Fin 3) := ua (Fin 3) (Fin 3) (isoToEquiv (Fin 3) (Fin 3) "
    "(fun (r : Fin 3) => finAdd 3 1 r) (fun (r : Fin 3) => finAdd 3 2 r) (fun (k : Fin 3) => refl) (fun (k : Fin 3) => refl))\n"
    "def back : Path (Fin 3) 0 0 := fun (i : I) => transp (fun (j : I) => loopRot1 @ (i /\\ ~j)) i0 "
    "(transp (fun (j : I) => loopRot1 @ (i /\\ j)) (~i) 0)\n",
    "(finAdd 3 2 (finAdd 3 1 (glue");
  /* S6b item 6, rule 2: the components 1 and 2 are different, so each
     part must come from its own component path. */
  failed += !json_contains("Sigma path prints each component",
    "def p : Path ((e : Fin 2) * Fin 3) (pack 1 2) (pack 1 2) := refl\n",
    "\"value\":{\"witness\":{\"refl\":1},\"payload\":{\"refl\":2}}");
  /* S6b item 6, rule 3 at a ua domain: the pointwise path at x goes from
     f x to g (s x), so the table starts at x = 0 with 1 (asymmetric). */
  failed += !json_contains("path of functions prints a table of paths",
    "def l : Path (Type 0) (Fin 3) (Fin 3) := ua (Fin 3) (Fin 3) (isoToEquiv (Fin 3) (Fin 3) (fun (r : Fin 3) => finAdd 3 r 1) (fun (r : Fin 3) => finAdd 3 r 2) (fun (k : Fin 3) => refl) (fun (k : Fin 3) => refl))\n"
    "def q : (z : Fin 1) -> PathP (fun (i : I) => (l @ i) -> Fin 3) (fun (x : Fin 3) => finAdd 3 x 1) (fun (x : Fin 3) => x) := "
    "fun (z : Fin 1) => uaArrow (Fin 3) (Fin 3) (Fin 3) (isoToEquiv (Fin 3) (Fin 3) (fun (r : Fin 3) => finAdd 3 r 1) (fun (r : Fin 3) => finAdd 3 r 2) (fun (k : Fin 3) => refl) (fun (k : Fin 3) => refl)) "
    "(fun (x : Fin 3) => finAdd 3 x 1) (fun (x : Fin 3) => x) (fun (w : Fin 3) => refl)\n"
    "def p : Path ((A : Type 0) * (A -> Fin 3)) (pack (Fin 3) (fun (x : Fin 3) => finAdd 3 x 1)) (pack (Fin 3) (fun (x : Fin 3) => x)) := "
    "fun (i : I) => pack (l @ i) ((q 0) @ i)\n",
    "\"payload\":{\"paths\":[{\"from\":1,\"to\":1,\"term\":");
  /* S6b item 6, rule 1 at the Sigma carrier: a payload-major order gives
     [0,3,1,4,2,5]. */
  failed += !json_contains("ua table at a Sigma carrier",
    "def C : Type 0 := (e : Fin 2) * Fin 3\n"
    "def lift : (x : Fin 3) -> Fin 6 := fun (x : Fin 3) => finCase 3 x 0 1 2\n"
    "def fwd : (p : C) -> Fin 6 := fun (p : C) => finCase 2 (witness p) (lift (payload p)) (finAdd 6 (lift (payload p)) 3)\n"
    "def bwd : (k : Fin 6) -> C := fun (k : Fin 6) => finCase 6 k (pack 0 0) (pack 0 1) (pack 0 2) (pack 1 0) (pack 1 1) (pack 1 2)\n"
    "def sec : (k : Fin 6) -> Path (Fin 6) (fwd (bwd k)) k := fun (k : Fin 6) => refl\n"
    "def retC : (e : Fin 2) -> (x : Fin 3) -> Path C (bwd (fwd (pack e x))) (pack e x) := fun (e : Fin 2) => fun (x : Fin 3) => refl\n"
    "def ret : (p : C) -> Path C (bwd (fwd p)) p := fun (p : C) => retC (witness p) (payload p)\n"
    "def u : Path (Type 0) C (Fin 6) := ua C (Fin 6) (isoToEquiv C (Fin 6) fwd bwd sec ret)\n",
    "\"value\":{\"from\":\"Sigma (e : Fin 2) (Fin 3)\",\"to\":\"Fin 6\",\"transport\":[0,1,2,3,4,5]}");
  /* Empty literal carriers still select the table rules. */
  failed += !json_contains("empty Fin universe path prints an empty table",
    "def p : Path (Type 0) (Fin 0) (Fin 0) := refl\n",
    "\"value\":{\"from\":\"Fin 0\",\"to\":\"Fin 0\",\"transport\":[]}");
  failed += !json_contains("empty Sigma base prints an empty transport table",
    "def C : Type 0 := (e : Fin 0) * Fin 2\n"
    "def p : Path (Type 0) C C := refl\n",
    "\"transport\":[]}");
  failed += !json_contains("empty Sigma fibers print an empty transport table",
    "def C : Type 0 := (e : Fin 2) * Fin 0\n"
    "def p : Path (Type 0) C C := refl\n",
    "\"transport\":[]}");
  failed += !json_contains("path of functions on Fin zero prints an empty table",
    "def f : Fin 0 -> Nat := fun (x : Fin 0) => 0\n"
    "def p : Path (Fin 0 -> Nat) f f := refl\n",
    "\"value\":{\"paths\":[]}");
  failed += !json_contains("path of functions on an empty Sigma prints an empty table",
    "def C : Type 0 := (e : Fin 2) * Fin 0\n"
    "def f : C -> Nat := fun (x : C) => 0\n"
    "def p : Path (C -> Nat) f f := refl\n",
    "\"value\":{\"paths\":[]}");
  /* Every Sigma fiber must be literal before either table rule applies.
     The second fiber here is infinite, beyond the finite first fiber. */
  failed += !json_contains("mixed Sigma carrier keeps the whole universe path",
    "def C : Type 0 := (e : Fin 2) * (finCase 2 e (Fin 1) Nat)\n"
    "def p : Path (Type 0) C C := refl\n",
    "\"value\":{\"refl\":\"Sigma ");
  failed += !json_contains("mixed Sigma function domain keeps the whole path",
    "def C : Type 0 := (e : Fin 2) * (finCase 2 e (Fin 1) Nat)\n"
    "def f : C -> Nat := fun (x : C) => 0\n"
    "def p : Path (C -> Nat) f f := refl\n",
    "\"value\":{\"refl\":\"fun x => 0\"}");
  /* Literal dependent fibers can have different sizes, including zero. */
  failed += !json_contains("dependent Sigma transport enumerates every fiber",
    "def C : Type 0 := (e : Fin 3) * (finCase 3 e (Fin 1) (Fin 0) (Fin 2))\n"
    "def p : Path (Type 0) C C := refl\n",
    "\"transport\":[{\"witness\":0,\"payload\":0},{\"witness\":2,\"payload\":0},{\"witness\":2,\"payload\":1}]}");
  failed += !json_contains("dependent Sigma function paths enumerate every fiber",
    "def C : Type 0 := (e : Fin 3) * (finCase 3 e (Fin 1) (Fin 0) (Fin 2))\n"
    "def f : C -> Nat := fun (x : C) => finVal 3 (witness x)\n"
    "def p : Path (C -> Nat) f f := refl\n",
    "\"value\":{\"paths\":[{\"refl\":0},{\"refl\":2},{\"refl\":2}]}");
  printf("path regressions: %d checked, %d failed\n", cases_run, failed);
  return failed != 0;
}
