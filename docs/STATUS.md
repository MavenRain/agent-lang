# agent-lang status

> Write here: one paragraph. Say which milestone the build implements, and
> which milestone it does not complete yet.

## Implemented

> Write here: one item for each feature that works. Name the former (F1 to
> F15), the core type or the core operation.

- <feature>.

## Remaining M0 work

> Write here: one numbered item for each missing feature. Say what each item
> needs first.

1. <work item>.

## Known limits

> Write here: one item for each limit of the current build, with its cause
> (a fuel budget, a host limit, a stack size).

- <limit>.

## Internal boundaries

> Write here: one paragraph for each module group. Say which module holds
> which job, which tests check which tables, and which types a program
> cannot use.

## S1: the coin-lang kernel port (2026-10-19)

S1 ports the coin-lang kernel at 61f1c8e: the interval, `PathP`, `transp`,
`hcomp`, Glue, `ua`, path constructors, `S1`, `Trunc1`, `Trunc0`, `Quot`,
`Fin`, `decEqFin` and `decEqNat`. The USER rulings for the build are K1 to
K7, each (a): K1 keep the kit `Flag` until S3; K2 the builtins `Empty` and
`absurd` replace `Fin 0` in the refutation type; K3 Fin-free rewrites (moot
after the Fin amendment); K4 the kit files take the non-Fin coin edits; K5
port `test/path-regressions.c` with a count line; K6 `dec-json-function`
uses `decEqNat`; K7 keep the coin `Fin` surface as it is. The USER admitted
`Fin` after the kickoff (`docs/PLAN.md`, section 1). The cut rules are
FinSet, Rand, couple, Bag, carrierSum and fiberMatch.

### Gate

| Suite | Count |
|---|---|
| examples | 20 checked, 0 failed |
| parse refusals | 11 |
| check refusals | 60 |
| evals | 48 |
| json builds | 20 |
| build refusals | 3 |
| regressions | 6 |
| printer regressions | 2 |
| path regressions | 67 checked, 0 failed (82 coin cases, 17 cut, 2 review cases) |

### Mutants

Each mutant is one edit, `make -B check`, then a restore that `shasum`
confirms. A kill at domain load means that every file fails at the same
domain definition before a guest test runs.

| Id | Site and edit | Result |
|---|---|---|
| M1 | `src/front/eval.c` `ival_neg`, the literal flip `^ 1u` to `^ 0u` | KILLED at domain load: SYSTEM_OVERLAP at the `Trunc1` boundary in every file |
| M2 | `src/front/eval.c` `path_apply`, the i0 end `return a;` to `return b;` | KILLED at domain load: SYSTEM_OVERLAP at the `Trunc0` boundary in every file |
| M3 | `src/front/eval.c` transp, `ival_is_end(face, 1)` to `ival_is_end(face, 0)` | KILLED at domain load: PATH_END at `JRefl` in every file |
| M4 | `src/front/eval.c` hcomp, `ival_is_end(face, 1)` to `ival_is_end(face, 0)` | KILLED at domain load: PATH_END at `trans` in every file |
| M5 | `src/front/eval.c` `if (!glue_true` to `if (glue_true` | KILLED at domain load: PATH_END at `ua` in every file |
| M6 | `domain/domain.lang` `ua`, `~i -> pack A e` to `~i -> pack B (idEquiv B)` | KILLED at domain load: PATH_END at `ua` in every file |
| M7 | `src/front/eval.c` fold at a path constructor, the stored end `val_iend(m, side)` to `val_iend(m, 0)` | SURVIVED: `make check` exit 0 |
| M8 | `domain/domain.lang` `squash1`, `i -> squashTo` to `i -> squashFrom` | KILLED: `examples/trunc.lang` `sq1` PATH_END, the eval `trunc truncAt`, the golden `trunc.json` |
| M9 | `src/front/eval.c` `fin_add`, the wrap `x >= k - y ? x - (k - y) : x + y` to `x + y` | KILLED: `examples/circle.lang`, `equiv.lang`, `glue.lang`, `quot.lang`, `trunc.lang`, `univ.lang` PATH_END; `test/check/ua-not-refl`; three evals |

M1 to M6 die before a guest test runs, so they do not meet the kickoff
standard (a kill from a guest test or a path regression). A weaker edit per
rule, one that the domain definitions do not use, is an S2 item. M7 is a
survivor: no test observes the ends that the fold stores for a path
constructor at a neutral interval (hypothesis: the tests fold `spin`,
`squash1` and `quotEq` only at closed ends, where the constructor reduces
first). A guest test for S2: a `Quot` fold with distinct ends under a
neutral interval, with a check of the i1 end.

### Builder deaths

The chunk A builder (wf-builder) died at launch twice on the API safeguard
classifier: Fable `req_011CgBAHrpn3NXSWJ8ZQ7QGE` and opus
`req_011CgBALrYk3Eurw3xY5qq4G`. The USER ruled a hand build in the main
loop (sessions 2 to 4).

### Carry-over items

- The ported comments keep the donor slice and ruling citations ("S4a,
  rulings R1, R2", "S5b item 7", "O11"). The name sweep does not flag them.
  A comment sweep is a USER call.
- `examples/equality.lang` line 1 names transport, and that def is gone.
- Chunk C changed `refute_core` from `Fin 0` to `Empty` (K2 on the eval
  side): the path regression `decEqFin inr round-trips` caught the
  mismatch with the check side.

### Review fixes

`decEqNat` refutations now select `Unit` or `Empty` with `flagIf`, matching
the `Flag` result of `natEq`. The new source round-trip regression failed
before the fix and passes after it.

Formal `hcomp` JSON retains its tube as source syntax alongside the face
and base. Previously, two circle paths with different tubes serialized
identically even though their transports returned 1 and 2. A new regression
requires distinct JSON for these distinct paths.

## S2: the orbit-lang QIIT and HIIT port (2026-10-19)

S2 ports the orbit-lang indexed families, mutual blocks and the family
eliminator from the donor commits 6fa6a15, 8dd82a3, 8b11846 and 676c7a8
(the diffs of the front files, not the files). The USER rulings Q1 to Q9
are each (a) (`docs/PLAN.md`, section 1). Chunk C writes this section in
full.

### Chunk A: grammar, core and check

Chunk A adds the grammar (`and`, the index telescope, the constructor
result, `elim`), the core fields and the Op `OP_ELIM`, the check of
indexed families, blocks, block folds and the eliminator, the domain
families `Vec` and the `Charter`/`Ratified` block, 6 check rows and 15
path regression cases. The eval of the folds and of `elim` is chunk B:
eval.c has `varied_arity`, `transp_data`, `block_ctors` and a `stuck_elim`
stub only.

| Suite | Count |
|---|---|
| examples | 20 checked, 0 failed |
| parse refusals | 11 |
| check refusals | 66 |
| evals | 48 |
| json builds | 20 |
| build refusals | 3 |
| regressions | 6 |
| printer regressions | 2 |
| path regressions | 82 checked, 0 failed |

Check rows (test/check, each TYPE_MISMATCH): indexed-fold-missing-case (a
Vec fold with the vnil and vcons cases and no vswap case),
block-fold-target (a Charter block fold with one target),
block-fold-missing-case (a Charter block fold without the ratify case),
elim-family-count (a Vec elim with two cases), elim-motive-index (a Vec
elim whose motive has no index binder), elim-block (an elim on a Charter
value, Q7 (a)).

Path regression cases (test/path-regressions.c, cases 68 to 82): an end
at another index (HIT_INDEX), a constructor without a result (HIT_INDEX),
a result head that is not the family (HIT_INDEX), a header that names a
later sort (BLOCK_ORDER), a field that names a later sort (accepted), a
block fold with an extra argument before the value (TYPE_MISMATCH), a
header fold of the sort under check (HIT_BOUNDARY), the block universe at
Type 0 (TYPE_MISMATCH) and at Type 1 (accepted), two higher-order index
telescopes (accepted), a block fold with the same parameter telescope
(accepted) and with a different one (TYPE_MISMATCH), a partial family
eliminator (accepted), mixed motive annotations (accepted).

Deviations:
- Method: the check.c diffs of 6fa6a15, 8dd82a3 and 676c7a8 went in by
  `patch -p1 -F3`, the rejects by hand. The 6fa6a15 `check_family`
  rewrite and the 676c7a8 `rule_elim` are hand-built: the orbit rule also
  covered `Sum` and `Unit`, which agent-lang does not have, so
  `elim_domain` has the family case only.
- The builtin table row of `elim` has `OP_FAMILY` in its `result` field.
  `RULE_ELIM` never reads that field: the result type of an `elim` comes
  from the motive. The field is a placeholder, not a rule input.
- A block sort under `elim` is refused by the generic `rule_elim` text
  ("elim needs a motive on a domain family outside a block"), as in
  orbit: only a family outside a block reaches `elim_on_family`.
- The 20 JSON goldens did not change when `Vec` and the block entered
  domain.lang, so no golden was regenerated.
- The orbit L/R row ("the same parameter telescope") checks its two folds
  and does not reduce them here: the block fold eval is chunk B.
- In the regression texts the orbit `Unit` and `unit` are `Nat` and `0`,
  and the orbit `Size`, `sz0` and `szS` are `Nat`, `0` and `succ`. The
  orbit `state` rows and `Prop` rows are dropped (no state, Q4 (a)).

### Chunk B: eval folds and elim

Chunk B ports the eval of the indexed and block folds (donor commits
8dd82a3 and 6fa6a15) and of the family eliminator (donor commit 676c7a8)
into `src/front/eval.c`. It adds 1 check row and 11 path regression cases.
`stuck_elim` is no longer a stub.

| Suite | Count | Change |
|---|---|---|
| examples | 20 checked, 0 failed | none |
| parse refusals | 11 | none |
| check refusals | 67 | 66 -> 67 |
| evals | 48 | none |
| json builds | 20 | none |
| build refusals | 3 | none |
| regressions | 6 | none |
| printer regressions | 2 | none |
| path regressions | 93 checked, 0 failed | 82 -> 93 |

Kickoff row (S2-kickoff.md, section 7, chunk B): eval.c has the 8dd82a3
hunks (`block_ctors`, `fold_res`, `field_sort`, `stuck_fold`, the base of
`fold_family`, `fold_path_field`, `fold_hcomp`, the targets first in
`print_value`) and the 676c7a8 hunks (`reduce_elim`, `stuck_elim`,
`elim_at`, `elim_path`, `elim_square`, `elim_family`, the `kan_apply`
branches, `reduce_op`). The new check row is elim-path-case (PATH_END):
a Vec elim whose vswap case does not meet the vcons image at i0.

Functions in eval.c:
- `fold_res`: the result type of a fold at FAMILY. It is CASES[NCASES],
  or, for a block of n sorts, the target of FAMILY among the n targets
  before the cases.
- `field_sort`: the sort of a recursive field. It is the head of the
  field type, or the head of the type under a path field.
- `stuck_fold` stores `fold_res` at args[ncases]. `fold_family` starts
  at the first constructor of the block, refuses a constructor of
  another family as INTERNAL, and folds a recursive field at
  `field_sort`. `fold_path_field` builds its CLOS_FOLD_PATH closure at
  `field_sort`. `fold_hcomp` composes at `fold_res`.
- `print_value` prints the targets of a block fold before the cases and
  omits RES. The finCase branch stays.
- `reduce_elim`, `stuck_elim`, `elim_at`, `elim_path`, `elim_square`,
  `elim_family`: the family eliminator. The `kan_apply` arms for
  CLOS_FOLD_PATH and CLOS_FOLD_FACE send a stuck elim to `elim_at`. The
  `reduce_op` OP_ELIM arm calls `reduce_elim`.

Path regression cases (test/path-regressions.c, cases 83 to 93):
1. indexed fold: index arguments are not parameters (accepted, refl
   check of the value).
2. block fold: Even/Odd one (accepted, refl check of the Odd fold).
3. block fold: Even/Odd two (accepted, a case uses the folded sibling
   field).
4. block fold: Box, a sibling recursive field at its target (accepted).
5. block fold: L/R, the folds reduce (accepted, refl checks of firstL
   and secondR).
6. transport: the fibre of a transport result (accepted).
7. block fold: the print order of the targets, Even -> Nat (round trip).
8. block fold: the print order of the targets, Odd -> Nat (round trip).
9. transport round trip: a constant line at Vec.
10. transport round trip: an index line.
11. transport round trip: Even/Odd.

Deviations:
- (a) No new round trip helper. `domain_roundtrip(label, domain, source)`
  already exists in test/path-regressions.c, and the 5 round trips use it.
- (b) `reduce_elim` refuses field 0 as INTERNAL. The orbit Sum and Unit
  branches are not ported, because agent-lang has neither. Any other
  field goes to `elim_family(field - 1)`.
- (c) The transport round trips (cases 9 to 11) print stuck `transp`
  terms. `transp_data` stays neutral for an indexed family and for a
  block (chunk A ruling Q8 (a)).
- (d) `print_value` keeps the agent-lang finCase branch (S5a). The donor
  has no such branch.
- The orbit test texts use `Nat` for `Size` and for `Unit`, `0` for
  `sz0` and for `unit`, and `succ` for `szS`. Fresh names (Vx, Nx,
  Even/Odd, Boxx/Itm, Lx/Rx) stand beside the domain names.

### Chunk C: examples, evals, goldens, the M7 test and the mutant run

Chunk C adds the S2 guest tests: three examples, the M7 defs in
`examples/quot.lang`, 13 eval rows, 3 JSON goldens, 2 path regression
cases, the `## S2 shapes` section of `formers/tcc-json.md` and the PLAN
section 8 row. It changes no source file beyond HEAD b11f1cb.

| Suite | Count | Change |
|---|---|---|
| examples | 23 checked, 0 failed | 20 -> 23 |
| parse refusals | 11 | none |
| check refusals | 67 | none |
| evals | 61 | 48 -> 61 |
| json builds | 23 | 20 -> 23 |
| build refusals | 3 | none |
| regressions | 6 | none |
| printer regressions | 2 | none |
| path regressions | 111 checked, 0 failed | 109 -> 111 (deviation 6: 93 -> 109 in HEAD) |

Examples:
- `examples/indexed.lang`: `Vec (Fin 2)` with `vswap`. `vecCount` folds
  into Nat, `vecSwapPath` is the vswap line from `vecTwo` to `vecSwapped`.
- `examples/mutual.lang`: the `Charter R` and `Ratified R` block.
  `adopted` is the block fold at Charter, `ratifiedRule` the same fold at
  Ratified, `ratifiedOne` a refl check.
- `examples/elim.lang`: `toTruncE` (a Quot elim into Trunc0), `vecMapE`
  (a dependent motive), `vecSwapE`, `swapAt0`, `swapAt1`, `vecCountE`,
  `vecIdE`, `vecIdPathE` (a path motive, the vswap square) and `hcompE`
  (the elim of a formal hcomp stays stuck).
- `examples/quot.lang` (the M7 guest test): `Eq4`, `toTruncEq`,
  `foldEqAt` and `foldEqAt1`.

Eval rows (`test/eval/expect.txt`, 13): `indexed vecTwoCount` 2,
`vecSwappedCount` 2; `mutual adoptedZero` 0, `adoptedOne` 7, `adoptedLaw`
7; `elim vecTwoCountE` 2, `vecTwo`, `swapAt0`, `swapAt1`, `vecSwapE` (the
vswap line), `quotThreeE` (`trunc0 (Fin 4) 2`), `quotEqAtE` (a constant
line) and `hcompE` (a stuck elim of a formal hcomp).

Path regression cases (test/path-regressions.c, cases 110 and 111):
1. `cong at i1 of a fold with a neutral path case reads the i1 end`: the
   M7 kill. `foldG` folds a Quot at a lambda-bound path case `g`; a cong
   line of `foldG g (quotEq ... a 2 r i)` at i1 is `trunc0 (Fin 4) 2`.
   Reader: `rule_cong` quotes the PAPP ends, the CORE_PAPP eval applies
   the path at i1, and the `path_apply` end rule returns the i1 end.
2. `block fold of a formal hcomp at the second sort composes at its own
   target`: the N4 kill. Even/Odd with the path constructor `oloop` on
   Odd, the targets Nat and `Fin 2`; a round trip of the fold of a formal
   hcomp at a lambda-bound `o : Odd` prints the hcomp type and checks it
   again.

Mutants (`S2-C-mutants.sh`, 13 edits, each `sd -F`, `make -B check`, a
`git checkout` restore that `shasum` confirms; the runner printed
`mutants: 13 run, 12 killed, 1 survived`, then case 111 killed N4 on a
hand run):

| Id | Site and edit | Result |
|---|---|---|
| N1 | `src/front/check.c` `branch_index`, `t->inst == fam` to `t->inst != fam` (1 of 1) | KILLED: `indexed family: an end at another index: expected HIT_INDEX` (TYPE_MISMATCH instead) |
| N2 | `src/front/check.c` `header_order`, `if (named)` to `if (named < 0)` (1 of 1) | KILLED: `block: a header names a later sort: expected BLOCK_ORDER` (REFUSE_REC instead) |
| N3 | `src/front/eval.c` `fold_family`, a recursive field folds at `family`, not at `field_sort` (1 of 1) | KILLED: `examples/mutual.lang` `ratifiedOne` INTERNAL, a fold of a constructor of another family |
| N4 | `src/front/eval.c` `fold_res`, the target of a block sort is always the first target (1 of 1) | SURVIVED the 110 cases; KILLED by case 111 (+1): the replay `TYPE_MISMATCH: expected a term of type Nat, found a term of type Fin 2` |
| N5 | `src/front/check.c` `elim_res`, the case result is the motive at the scrutinee type `st` (1 of 1) | KILLED: `examples/elim.lang` `vecMapE` TYPE_MISMATCH, expected `Vec B n`, found `Vec B 0` |
| N6 | `src/front/eval.c` `elim_family`, `elim_path` only on a NULL acc, else `stuck_elim` (1 of 1) | KILLED: `examples/elim.lang` `vecIdPathE` FIN_CASE at the case 0 of Fin 2, wrapping PATH_END |
| M7 | `src/front/eval.c` `fold_path`, the stored end `val_iend(m, side)` to `val_iend(m, 0)` (1 of 1) | KILLED by case 110: `atEnd` FIN_CASE, TYPE_MISMATCH `trunc0 (Fin 4) 2` against `trunc0 (Fin 4) 0` |
| M1w | `src/front/eval.c` CLOS_SIGMA_DOM, `ival_join(m, phi, ival_neg(m, r))` to `ival_join(m, phi, r)` (1 of 4) | KILLED: `examples/kan.lang` `transportSigma` PATH_END at i1 |
| M2w | `src/front/eval.c` `fold_path`, `ends[side]` to `ends[1 - side]` (1 of 2; `fold_square` is site 2) | KILLED by case 110, the same observation as M7 |
| M3w | `src/front/eval.c` transp rule 1, the end test 1 to 0 for a constructor base only (2 of 3; the plain flip dies at domain load, `JRefl`) | KILLED: `examples/trunc.lang` `truncUa` PATH_END at i1 |
| M4w | `src/front/eval.c` hcomp rule 1, the end test 1 to 0 at a family type only (2 of 2; the plain flip dies at domain load, `trans`) | KILLED: `examples/circle.lang` `loop2` PATH_END at i0; also `elim.lang` `hcompE`, `trunc.lang` `truncHcomp` |
| M5w | `src/front/eval.c` `reduce_glue_elem`, `glue_true` to `!glue_true` (1 of 2) | KILLED: `examples/circle.lang` `loopT` PATH_END at i1; also `uaBetaPlus3`, `truncUa` |
| M6w | `domain/domain.lang` `finSub`, `finAdd n a (finNeg n b)` to `finAdd n a b` (1 of 1) | KILLED: `examples/fin.lang` `subAt` PATH_END, 7 for 8 |

Every kill comes from a guest test or a path regression, so the S1 open
item (M1 to M6 died at domain load) is closed by the weaker sites M1w to
M6w. M7, the S1 survivor, is killed by case 110.

Deviations:
1. Rule R9 refuses `(i : I) -> T` as a Pi type (INTERVAL_TYPE), so the
   kickoff rows that bind an interval are Path types in the orbit shape:
   `vecSwapE`, `swapAt0`, `swapAt1`, `quotEqAtE`, `foldEqAt` and
   `foldEqAt1` (`foldEqAt a r @ i1`). The eval prints are unchanged.
2. `vecIdPathE` (the path motive with the vswap square) passed on the
   first attempt; no `s1IdPath` fallback.
3. The JSON emitter `family_json` did not admit the indices of an indexed
   family type; a one-line edit, now in HEAD through the USER commit.
4. `examples/elim.lang` defines `vecXY`, `vecYX` and `r13 : Par 1 3`
   beside the kickoff defs, for `hcompE` and the `vecSwapE` ends.
5. `examples/mutual.lang` failed to build JSON (INTERNAL at `charterOne`):
   the json.c `fi->recursive ? type : ...` field type gave the sibling
   sort `Ratified` the `Charter` type. HEAD has the fix (`ft =
   eval_core(...)` on every field); chunk C only regenerated
   `test/json/mutual.json` (470 bytes).
6. The chunk C precondition differed: HEAD b11f1cb is the USER commit of
   chunk B with review fixes beyond the staged tree 0451fd14 (check.c
   +113, eval.c 2, json.c 7, and 16 path regression cases, 93 -> 109).
7. Only a cong line kills M7: `conv_loop` ignores the stored ends, the
   printer prints `dom @ arg`, JSON refuses stuck terms, and `val_isubst`
   rebuilds the PAPP without `path_apply`. `json_contains` and a round
   trip cannot kill it.
8. The N4 killer needs a path constructor on the second sort and a round
   trip. `hcomp_data` sends a family without path constructors to
   `hcomp_stuck`, a fold of that neutral is a `stuck_fold` whose RES the
   printer omits, and only `fold_hcomp` reads `fold_res` in an observable
   way (the hcomp type). A first attempt without `oloop` survived. The
   domain has no `Unit`, so the two targets are Nat and `Fin 2`.

## S3: the base span (2026-10-19)

### Chunk A: types and check

Chunk A adds the base types of PLAN section 5 with their checks: `Text`
(the `"..."` literal with the escapes `\" \\ \n \t \xHH`, `CORE_TEXT`,
`VAL_TEXT`), `Int64` (a `CORE_NAT` leaf marked `op = OP_INT64`, `VAL_INT64`),
a Nat literal at or above 2^64 (`CORE_BIG`, `VAL_BIG`, little-endian base
2^32 limbs), the 18 operation names of rows 74 to 75 and 162 (`textAppend`
to `jsonRender`), `Bool = Fin 2` with `true`, `false` and `boolIf` in the
domain (Q1 (a)), and the domain families `Json`, `JsonList`, `JsonFields`
(one block) and `Log` with `logLength` (Q7 (a), Q8 (a)). `Flag`, `flagYes`,
`flagNo` and `flagIf` are gone: `natEq` and `natLe` return `Fin 2`, and
the filter test is a `Fin 2`. The eval of every new operation is chunk B:
`reduce_op` returns a `VAL_STUCK` for each. json.c emits an `Int64` literal
as a number so `jnum (inl n)` keeps its declared payload; `Text` JSON
emission and big Nat decimal printing are left for chunk B.

| Suite | Count | Change |
|---|---|---|
| examples | 23 | 0 (seven rewritten: entries, formers, monad, laws, fin, algebra, inventory) |
| parse refusals | 13 | +2 (`text-unterminated.lang LEX_STR_OPEN`, `text-escape.lang LEX_STR_ESCAPE`) |
| check refusals | 71 | +4 (`text-literal-type.lang`, `text-slice-arity.lang`, `int64-literal-range.lang`, `json-entry-type.lang`) |
| evals | 61 | 0 (rows 13 to 17 and 25 change their outputs) |
| json goldens | 23 | 0 (formers, inventory, laws, monad regenerated) |
| build refusals | 3 | 0 |
| regressions | 6 | 0 |
| printer regressions | 2 | 0 |
| path regressions | 119 | +8 (limb allocation failure, Json number payloads, Int64 JSON, and literal composition) |

Removed: the sample domain (`Color`, `Item`, `Stack`, `Lot`, `lineTotal`,
`isRed`, `total`, `redTotal`, `depth`), `Flag` and its three operations,
the `LEX_NAT_RANGE` refusal (a span at or above 2^64 lexes with
`nat = UINT64_MAX` and its digits kept), the `Entry` flag fields.

Deviations:

- `string.lang` now reads `'bolt'` (code `LEX_CHAR` stays) and
  `nat-range.lang` reads `def x : Fin 18446744073709551616 := 0` (code
  `FIN_RANGE`): the old inputs lex in S3.
- The two dependent-map sites (`dependent-result.lang`,
  `path-regressions.c` "dependent congruence") use the `Charter`/`Ratified`
  block, because `Vec Nat n` has no closed map `(n : Nat) -> Vec Nat n`.
  `regressions.js` keeps `Vec Nat n` at closed values.
- `Json` is a block of three families. PLAN row 162 writes `jarr (List
  Json)` and `jobj (List (Sigma Text Json))`, but the family rule takes
  direct recursion only (`DOMAIN_FAMILY`), so `JsonList` and `JsonFields`
  carry the lists. `jnum` carries `Sum Int64 Text` (Q7 (a)).
- An `Int64` literal is a `CORE_NAT` with `op = OP_INT64`, not a new
  `CoreKind`; `quote` and `eval` read the mark.
- `Bool` is reserved for program declarations only (`is_reserved` in
  `check_decl`), since the domain now defines it. `Text` left `RESERVED`:
  it is a core name.
- The examples hold no `Text` value at the top level: json.c cannot emit
  one before chunk B. `inventory.lang` uses `jnum` records and a priced
  `Sigma`.

Review fixes: a failed limb allocation reports `OOM` instead of checking
the literal as `UINT64_MAX`. Literal composition uses the same head rule
for `Text`, `Int64`, big Nat and word Nat values. The Json number tests
accept both sum branches and refuse a bare Nat payload.

### Chunk B: eval and JSON

Chunk B evaluates the 18 operations of chunk A and emits the S3 values.
A Nat is a word (`VAL_NAT`) below 2^64 and little-endian base 2^32 limbs
(`VAL_BIG`, `src/front/big.c`) at or above it: `natAdd`, `natSub`,
`natMul`, `succ`, `natEq` and `natLe` promote and demote, so each number
has one normal form, and the `EVAL_OVERFLOW` trap at the Nat operations
is gone. `textAppend`, `textEq`, `textSlice` (byte offsets, clamped),
`textLength`, `textUtf8` and `natShow` work on bytes. The Int64
arithmetic wraps; `int64Div` and `int64Rem` give `none` at a zero divisor
and at the minimum divided by -1; `int64ToNat` and `natToInt64` give
`none` out of range. `jsonParse` and `jsonRender` (`src/front/jsontext.c`)
read and write the domain `Json` tree. `langc eval` prints a Text, Int64,
big Nat, Sum, Option or constructor result in the surface syntax. json.c
writes a valid UTF-8 `Text` as a JSON string, any other `Text` as
`{"bytes":"HEX"}`, and a big Nat as a bare number (formers/tcc-json.md
`## S3 shapes`).

| Suite | Count | Change |
|---|---|---|
| examples | 26 | +3 (`text.lang`, `int64.lang`, `json.lang`) |
| parse refusals | 13 | 0 |
| check refusals | 71 | 0 |
| evals | 94 | +33 (11 text, 14 int64, 5 json, 3 entry rows; rows 2 and 3 give `18446744073709551616`) |
| json goldens | 26 | +3 (`text.json`, `int64.json`, `json.json`) |
| build refusals | 3 | 0 (`trap.lang` is a fuel trap, `EVAL_FUEL`) |
| regressions | 6 | 0 (the source list gets `big` and `jsontext`) |
| printer regressions | 2 | 0 |
| path regressions | 144 | +25 (four base-type paths and 21 review regressions) |

Deviations:

- (a) The path regressions start at 119 (the chunk A commit has its
  review fixes), so the initial target is 123, not 115. The review adds
  21 cases for a final count of 144.
- (b) `jnum` carries `Sum Int64 Text` (Q7 (a)): `jsonParse` gives `jnum
  (inl n)` for an integer that fits Int64 and `jnum (inr text)` for each
  other number (a fraction, an exponent, out of range). `jsonRender` writes
  `inl n` in decimal and `inr t` as it is.
- (c) A big Nat in a JSON document is a bare number of any length (Q6
  (a)), not a decimal string.
- (d) `test/emit/trap.lang` is a fuel trap with the row `trap.lang
  EVAL_FUEL` (Q9), since a Nat sum does not trap now.
- (e) The trap code of `ready` in json.c is `EVAL_TRAP` "the instance
  value is a trap", not `EVAL_OVERFLOW` (docs/host/README.md and
  CAPABILITY.md have the new name).
- (f) The fuel trap is the identity fold `fold (fun (acc : Nat) => acc) 0
  18446744073709551615` (0.35 s): a `natAdd` fold uses all of the 1 GiB
  arena (`OOM`) before 20000000 steps, since each step allocates a value.
- (g) `test/regressions.js` compiles its own source list, so it gets
  `big` and `jsontext` beside the Makefile `FRONT`.
- (h) `langc eval` prints a result that is not a Nat in the surface
  syntax (`"abcd"`, `-5`, `some 3`, `inl "byte 3: expected a value"`).
  The arguments stay Nat (`src/main.c` does not change).
- (i) `text_utf8_valid` is in jsontext.c: `textUtf8` in eval.c and the
  `Text` emission in json.c use it.
- (j) `jsonParse` keeps the bytes at or above 0x80 as they are (no UTF-8
  test), keeps duplicate keys in order, gives `jnum (inl 0)` for `-0`,
  refuses a lone surrogate escape and stops at 512 levels of nesting
  (`inl "byte N: the nesting is deeper than 512 levels"`; `jsonRender`
  gives `EVAL_DEPTH` there). `jsonRender` writes `"`, `\` and the bytes
  below 0x20 as escapes (`\u00xx` in lowercase).
- (k) Each S3 operation is stuck at a glue element argument.
- (l) An entry has a first-order result (`check.h`), so a function to
  `Text` is not an entry. The entries of `text.lang` (`sumDigits`,
  `tagLength`) and `json.lang` (`renderedLength`) build a `Text` and give
  its length; `int64.lang` has closed instances only.
- (m) No `text-slice-bounds.lang` check row: `textSlice` clamps its
  offsets, so no slice is an error.
- The `trap` branch of `test/gate.sh` (the `EVAL_OVERFLOW` eval rows) is
  dead: no eval row gives `trap` now. It stays.


Review fixes: `textAppend` copies each input's own length. JSON numbers
with leading zeros are rejected, and JSON primitives validate the complete
domain schema before constructing values. `decEqNat` compares and quotes
full large numbers. Fold and unfold accept large Nat bounds and preserve
the remaining bound when unfolding becomes neutral. Negative Int64 normal
forms print as source expressions, including the minimum integer; a closed
Int64 eval still prints a signed decimal. The C regression suite adds 21
review cases, including carry and borrow boundaries, and passes all 144
cases under TinyCC and under Clang AddressSanitizer and UndefinedBehaviorSanitizer.

### Chunk C: examples, mutants and docs

Chunk C adds the S3 guest tests, the mutant run and the docs. It changes
no source file beyond HEAD 0cd6954. `examples/nat.lang` (15 eval rows) and
`examples/log.lang` (10 eval rows) are new, with the goldens
`test/json/nat.json` and `test/json/log.json`. `text.lang` gets the rows
`acuteLength`, `halfAcute`, `tailAcute` and `sameIsTrue`, and `json.lang`
gets `maxNumber`, `overNumber`, `minNumber` and `underNumber`; `text.json`
and `json.json` are regenerated. The check row `log-entry-type.lang
TYPE_MISMATCH` refuses `logAppend logEmpty 3`. The three new path
regressions are the Json block fold (a tree of 4 nodes), the Log elim (it
agrees with `logLength` and gives 2) and `json_nesting` (514 open
brackets give `inl "byte 513: the nesting is deeper than 512 levels"`).
README.md gives the base types, and the PLAN section 8 row S3 is DONE.

| Suite | Count | Change |
|---|---|---|
| examples | 28 | +2 (`nat.lang`, `log.lang`) |
| parse refusals | 13 | 0 |
| check refusals | 72 | +1 (`log-entry-type.lang`) |
| evals | 127 | +33 (15 nat, 10 log, 4 text and 4 json rows) |
| json goldens | 28 | +2 (`nat.json`, `log.json`; `text.json` and `json.json` regenerated) |
| build refusals | 3 | 0 |
| regressions | 6 | 0 |
| printer regressions | 2 | 0 |
| path regressions | 147 | +3 (the Json block fold, the Log elim, `json_nesting`) |

Mutants (`S3-C-mutants.sh`, outside the repo, 15 edits; each edit is a
Python string replace that must apply one time, then `make -B check`,
then a `git checkout` restore that `shasum` confirms). Run 1, before the
survivor tests, printed `mutants: 15 run, 12 killed, 3 survived`: M4, M14
and M15 survived. Then the rows `tailAcute`, `bigLessSame` and
`bigLeSame` and the `json_nesting` path regression were added. Run 2, all
15 edits again, printed `mutants: 15 run, 15 killed, 0 survived`. A guest
test or a path regression kills each mutant; no mutant stops at domain
load.

| Id | File | Edit | Killer | Result |
|---|---|---|---|---|
| M1 | `src/front/eval.c` | `textAppend`: the operands swap places | eval `text greeting`: `"worldhello, "` | KILLED |
| M2 | `src/front/eval.c` | `textLength` counts code points, not bytes | eval `text acuteLength`: 1 for 2 | KILLED |
| M3 | `src/front/eval.c` | `textSlice`: the end offset is not clamped | eval `text clamped` (deviation (g)) | KILLED |
| M4 | `src/front/jsontext.c` | `text_utf8_valid`: `c < 0x80u` to `c < 0xc0u` (a lone continuation byte is valid) | eval `text tailAcute`: 1 for 0 | SURVIVED run 1, KILLED run 2 |
| M5 | `src/front/eval.c` | `int64Add` stops at the maximum and does not wrap | eval `int64 wrapped`: 9223372036854775807 for -9223372036854775808 | KILLED |
| M6 | `src/front/eval.c` | `int64Rem`: the sign of the result flips | eval `int64 remainder`: `some 1` for `some (int64Neg 1)` | KILLED |
| M7 | `src/front/big.c` | big add: the carry out of the top limb is lost | eval `entries addPrice`: 0 for 18446744073709551616 | KILLED |
| M8 | `src/front/eval.c` | Nat demote: a result of two limbs below 2^64 stays `VAL_BIG` | `examples/nat.lang` `demotePath`: PATH_END | KILLED |
| M9 | `src/front/jsontext.c` | `jsonParse`: a positive 2^63 fits Int64 | eval `json overNumber`: `jnum (inl ...)` for `jnum (inr "9223372036854775808")` | KILLED |
| M10 | `src/front/jsontext.c` | `jsonRender`: no escape for `"` | eval `json escaped` | KILLED |
| M11 | `src/front/check.c` | the Int64 literal bound is 2^63, not 2^63 - 1 | check `int64-literal-range.lang`: exit 0 for TYPE_MISMATCH | KILLED |
| M12 | `src/json.c` | a Text that is not valid UTF-8 goes out as a JSON string | the golden `test/json/text.json` | KILLED |
| M13 | `src/front/check.c` | `textEq` gives Nat, not `Fin 2` | `examples/text.lang` `sameText`: TYPE_MISMATCH | KILLED |
| M14 | `src/front/jsontext.c` | `JSON_DEPTH_MAX` 512 to 511 | path regression `json_nesting`: the error at byte 512 | SURVIVED run 1, KILLED run 2 |
| M15 | `src/front/big.c` | `big_cmp`: the sign of a limb comparison flips | eval `nat bigLessSame`: 1 for 0 | SURVIVED run 1, KILLED run 2 |

Deviations:

- (a) The chunk B commit has 21 review regressions, so the path
  regressions start at 144 and the target is 147, not 126.
- (b) The review case "big arithmetic carries and borrows through every
  limb" already has the M8 regression `def d : Path Nat (natSub
  18446744073709551616 1) 18446744073709551615 := refl`, so chunk C does
  not copy it. `demotePath` in `nat.lang` is the guest copy, and it kills
  M8 first.
- (c) `elim` refuses a block sort (`elim-block.lang`), so the Json path
  regression is the block fold.
- (d) The kickoff killer for M4, `textUtf8 (textSlice "\xc3\xa9" 0 1)`,
  is a lead byte without its continuation byte, not a lone continuation
  byte. Thus M4 survived run 1. The row `tailAcute` (`textSlice
  "\xc3\xa9" 1 2`) kills it. The row `halfAcute` stays.
- (e) The M9 and M10 sites are in `src/front/jsontext.c`, not in
  `eval.c`.
- (f) The first M12 edit (`v->nat == v->nat`) stopped at the clang
  warning `-Wtautological-compare`, not at a guest test. The edit is now
  `text_utf8_valid(...) || v->kind == VAL_TEXT`.
- (g) M3 reads past the end of the text, so its log has a NUL byte. The
  shell drops the NUL, and the log shows `"o"` on the two sides; the
  runner reads the logs with `rg -a`.
- (h) M14 (512 to 511) survived run 1. The new `json_nesting` regression
  kills it: under the mutant the error is at byte 512.
- (i) M15 (the `big_cmp` limb sign) survived run 1, because `bigLess`
  compares two different limb counts. The new rows `bigLessSame` and
  `bigLeSame` compare two Nats with the same limb count and kill it.
- (j) `json.lang` `minNumber` prints `inr (jnum (inl (int64Add
  9223372036854775807 1)))`: the chunk B review prints a negative Int64
  as source.

## S4: coinductive families (2026-10-20)

### Chunk A: grammar, core and check

Chunk A adds the keyword `cofamily` (`TOK_KW_COFAMILY`, `DECL_COFAMILY`),
the check of a cofamily declaration, and the check rules of `unfold` at a
cofamily and of `out` (USER rulings Q1 to Q10, all (a)). `cofamily C ps :=
F ps` declares the greatest fixed point of the layer family F at its last
parameter X. `check_cofamily` refuses a body that is not a domain family
at the parameters of C in order, and a layer that has an index, is in a
block, is above `Type 0`, has no carrier `X : Type 0`, has a different
parameter type or has a path constructor (`CODATA_FORM`). The body resolves
the layer and parameter names in their local scope, so shadowing cannot
change the layer or repeat an earlier parameter. It refuses a field that uses X in a
position that is not positive (`CODATA_POSITIVE`): a field does not use X,
is X, or is a Pi chain with the codomain X and no X in a domain. A program
cannot declare a cofamily (`REFUSE_DATA`). `FamilyInfo` records the
cofamily flag and the layer. `unfold g s` at a cofamily hint takes 2
arguments (`OP_COUNFOLD`, `g : S -> F ps S`, S the type of the seed);
without a hint it needs a type (Q8). The kit `unfold` at a `List` hint
keeps 3 arguments. `out x : F ps (C ps)` for `x : C ps` (`OP_OUT`). `fold`
and `elim` refuse a cofamily first (`CODATA_FOLD`). The domain adds
`StreamF` and `Stream`, `ColistF` and `Colist`, `MealyF` and `Mealy` (Q10).
The eval is chunk B: `reduce_op` keeps `unfold g s` as a canonical value
and returns a `VAL_STUCK` for each `out`. json.c puts both operations with
the other canonical values; the `JSON_VALUE` refusal is chunk B.
Coiteration retains its result and seed types through evaluation and
quotation. The printer uses typed identities for seeds and for coiterations
in inference positions, including the argument of `out`, so these values
print as source that checks again.

| Suite | Count | Change |
|---|---|---|
| examples | 28 | 0 |
| parse refusals | 14 | +1 (`cofamily-no-body.lang PARSE_EXPECT`) |
| check refusals | 78 | +6 (`codata-program.lang REFUSE_DATA`, `codata-step.lang` and `codata-out.lang TYPE_MISMATCH`, `codata-fold.lang` and `codata-elim.lang CODATA_FOLD`, `codata-bisim.lang PATH_END`) |
| evals | 127 | 0 |
| json goldens | 28 | 0 |
| build refusals | 3 | 0 |
| regressions | 6 | 0 |
| printer regressions | 2 | 0 |
| path regressions | 165 | +18 (6 `CODATA_FORM` and 2 `CODATA_POSITIVE` by `domain_refuses`, Pi chain and dependent layers, `Mealy`, the type of `out`, and round trips for coiterations, `out`, seeds, binders and path endpoints) |

Deviations:

- `unfold` at a cofamily is a new operation (`OP_COUNFOLD`: inst is the
  cofamily, the arguments are g and s), not a new Value kind. `OP_COUNFOLD`
  and `OP_OUT` are at the end of the `Op` enum, and `out` is the last
  BUILTINS row.
- `rule_fold` sends a cofamily to `fold_family` before the arity test of a
  family fold, thus `fold` refuses a cofamily (`CODATA_FOLD`) at each
  number of arguments. `elim` keeps the builtin arity 3 before `rule_elim`:
  `codata-elim.lang` gives a motive, one case and the stream. An `elim`
  with 2 arguments at a cofamily stops at `TYPE_ARITY`.

### Chunk B: eval and JSON

Chunk B adds the `out` step, the transp rule at a cofamily and the JSON
refusal of a cofamily value. `out (unfold g s)` spends 1 fuel and computes
r := g s (Q4). When r is a constructor of the layer F ps S, the result is
the same constructor of F ps (C ps). `out_field` changes an X field v to
`unfold g v` (`counfold_at`), changes a Pi chain field f to
`fun r1 ... rn => unfold g (f r1 ... rn)` and keeps a field without X. X is
the core variable at index i + d in the type of field i under d Pi binders,
as in `codata_positive`. `out` of a neutral argument or of a formal hcomp,
and `out` with a neutral r, are stuck (`VAL_STUCK`). `reduce_out` is its own
arm of `reduce_op`. The conversion of two unfolds and of two stuck `out`
values uses the structural arm of chunk A: no step runs and there is no
eta (`codata-bisim.lang` gives `PATH_END`). transp at a cofamily along a
constant line gives the base (regularity). Along a line that changes, it is
neutral. In json.c, a cofamily type is not an instance type, a def at a
cofamily type is not in the document, and an instance that holds a
cofamily value stops the build with `JSON_VALUE` (`a coinductive value`,
Q7). `formers/tcc-json.md` `## S4 shapes` records these rules. An evaluated
`out nats` and `fun x => out x` print as source that checks again, thus the
printer does not change.

| Suite | Count | Change |
|---|---|---|
| examples | 28 | 0 |
| parse refusals | 14 | 0 |
| check refusals | 78 | 0 |
| evals | 127 | 0 |
| json goldens | 28 | 0 |
| build refusals | 4 | +1 (`codata.lang JSON_VALUE`) |
| regressions | 6 | 0 |
| printer regressions | 2 | 0 |
| path regressions | 171 | +6 (one layer, two layers, the Pi chain map at depth 1 (`Mealy`) and depth 2, the stuck `out` of a variable with a round trip, transp along a constant and a changing `Stream` line, and the fuel of the second `out`) |

Deviations:

- The base is the USER commit e711b97 (tree 95265d81), not the chunk A tree
  fd7ac4f0 of the hand-off. The USER added 10 path regressions to chunk A,
  thus the chunk B count is 171 (165 + 6), not 161.
- A Pi chain field maps through a new closure tag `CLOS_OUT_PI` at the end
  of `ClosTag`. Its arguments are the unfold u and the field f, and its
  field count is the number of binders that remain. The last binder gives
  `unfold g (f r1 ... rn)`. There is no new Value kind.
- transp at a cofamily tests the line with `line_constant`: the conversion
  of the head at a fresh interval variable with the head at i0, at the
  level `KAN_FRESH_BASE + KAN_FRESH_MASK + 1`. A constant line gives the
  base. A line that changes stays neutral.
- The fuel regression cannot use a fuel of 1, because `eval_core` spends 1
  fuel at each core node. `out_fuel` measures K, the cost of `step` at a
  variable (there `out` is stuck before the spend), and A, the cost of g s
  at the seed of the tail. The second `out` gives a layer with K + A + 1
  fuel and stops with `EVAL_FUEL` at K + A. A scratch copy of eval.c without
  the spend (kickoff mutant M2) fails this case and only this case.
