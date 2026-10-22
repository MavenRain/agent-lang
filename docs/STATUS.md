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
- S4 chunk C: the printer drops a typed identity in the body of an
  `unfold` step. The printed form of `colist.lang` `three` does not check
  again (`TYPE_INFER: nil needs a declared type List A`), thus `three` has
  no eval row.

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

### Chunk C: examples, goldens, path regressions and mutants

Chunk C adds the S4 guest tests, the mutant run and the docs. It changes
no source file beyond HEAD 95ceac6. `examples/stream.lang` (10 eval rows),
`examples/colist.lang` (8 eval rows) and `examples/mealy.lang` (6 eval
rows) are new, with the goldens `test/json/stream.json`,
`test/json/colist.json` and `test/json/mealy.json`. `stream.lang` makes
`nats` by `unfold` and reads it with `headS`, `tailS`, `take` and `nth`.
`mapS` and `zipS` make a new stream by `unfold`, with the old stream as the
seed. `colist.lang` makes a finite colist from a `List` (`fromList`, a fold
that keeps the tail as the seed) and an infinite colist (`upFrom`).
`takeC` stops at the end of a finite colist. `mealy.lang` gives the machine
`counter` and runs it on a `List` of inputs (`run`, `final`). The last def
of each example is a `refl` that needs the `out` step. `langc eval` gave
each eval row before the row went into `test/eval/expect.txt`. The two new
path regressions are a `refl` of two unfolds after a `take` (`zerosA` and
`zerosB` of `codata-bisim.lang`, 3 entries: accepted) and a `refl` of
`unfold g` at the seeds `natSub 1 1` and `natAdd 0 1` (`PATH_END`). Each
new path regression failed one time in a scratch copy with a bad
expectation. README.md gives the coalgebras, and the PLAN section 8 row S4
is DONE.

| Suite | Count | Change |
|---|---|---|
| examples | 31 | +3 (`stream.lang`, `colist.lang`, `mealy.lang`) |
| parse refusals | 14 | 0 |
| check refusals | 78 | 0 |
| evals | 151 | +24 (10 stream, 8 colist and 6 mealy rows) |
| json goldens | 31 | +3 (`stream.json`, `colist.json`, `mealy.json`) |
| build refusals | 4 | 0 |
| regressions | 6 | 0 |
| printer regressions | 2 | 0 |
| path regressions | 173 | +2 (a `refl` of two unfolds after a `take`, a `refl` of g at 0 and g at 1 after eval) |

Mutants (`S4-C-mutants.sh`, outside the repo, 13 edits; each edit is a
Python string replace that must apply one time, then `make -B check`, then
a `git checkout` restore that `shasum` confirms). The results file gives
`mutants: 13 run, 13 killed, 0 survived`, with M10 from a rerun
(deviations below). The gate stops make at `test/gate.sh` when an example,
an eval row or a golden fails, thus a mutant that fails a guest test does
not reach the path regressions. The killer column gives the first FAIL
line. For M9, M11 and M13 the kickoff killer did not kill the mutant or
was not reached (deviations below).

| Id | File | Edit | Killer | Result |
|---|---|---|---|---|
| M1 | `src/front/eval.c` | `out_field`, X field: the layer keeps the old seed (`unfold g s`, not `unfold g v`) | `examples/colist.lang` `toEnd`: PATH_END | KILLED |
| M2 | `src/front/eval.c` | `reduce_out`: no fuel spend before `g s` | path regression `out_fuel` ("the second out traps when the fuel is 1 step short") | KILLED |
| M3 | `src/front/eval.c` | `out_field`, Pi chain field: `unfold g f`, not `fun r => unfold g (f r)` | `examples/mealy.lang` `runAgrees`: INTERNAL (an application of a value that is not a function) | KILLED |
| M4 | `src/front/eval.c` | `out_field`, field without X: the field changes as an X field | `examples/colist.lang` `toEnd`: PATH_END | KILLED |
| M5 | `src/front/eval.c` | conversion of two unfolds: compare the seeds only | check `codata-bisim.lang`: exit 0 for PATH_END | KILLED |
| M6 | `src/front/eval.c` | conversion of two unfolds: compare the steps only | path regression "unfold: refl of g at 0 and g at 1 after eval": no PATH_END | KILLED |
| M7 | `src/front/check.c` | `check_cofamily`: accept X in the domain of a Pi field | path regression "cofamily: X in the domain of a Pi field": no CODATA_POSITIVE | KILLED |
| M8 | `src/front/check.c` | `check_cofamily`: accept an indexed layer | path regression "cofamily: an indexed layer": no CODATA_FORM | KILLED |
| M9 | `src/front/check.c` | `rule_out`: `out x` has the type `C ps`, not `F ps (C ps)` | `examples/colist.lang` `takeC`: CODATA_FOLD | KILLED |
| M10 | `src/front/check.c` | `fold_family`: the cofamily test is `f->cofamily && f->ctor_count > 4096u` (false for each domain cofamily) | check `codata-fold.lang`: TYPE_NOT_FUNCTION for CODATA_FOLD | KILLED |
| M11 | `src/front/check.c` | `rule_unfold`, cofamily form: check g at `S -> C ps`, not `S -> F ps S` | `examples/colist.lang` `fromList`: TYPE_MISMATCH | KILLED |
| M12 | `src/json.c` | an instance that holds a cofamily value goes out as `null` | build refusal `codata.lang`: exit 0 for JSON_VALUE | KILLED |
| M13 | `src/front/eval.c` | printer: `unfold` prints the seed before the step | eval `stream nats` | KILLED |

Deviations:

- The 5 entries with arguments (`nth`, `nthDoubled`, `countThree`,
  `startAt`, `afterOne`) give a `Nat`. `entry_of` needs a first-order
  result, and a `List` result is not one (`EVAL_ARGS`, "not an entry").
- `stream.lang` has no row `outNats`. A def at `StreamF Nat (Stream Nat)`
  holds a stream, thus `langc build` refuses it (`JSON_VALUE`, Q7). The row
  `nthDoubled 7 => 14` is in its place. `colist.lang` `outEmpty` (`cnil`)
  holds no colist, thus it builds.
- `colist.lang` has no eval row for `three`. Its printed form does not
  check again (`TYPE_INFER: nil needs a declared type List A`): the readback
  of the step body drops the typed identity `(fun (z : List Nat) => z) nil`
  of the source. The printer puts typed identities only at seeds and at
  coiterations. This limit is older than chunk C, and chunk C does not fix
  it (section S1, `### Carry-over items`).
- Kickoff M5 input: `zerosA` and `zerosB` of `codata-bisim.lang` give the
  same 5 entries by `take` (each is `cons 0 (... nil)`), and a `refl` of the
  two lists accepts. The first new path regression keeps this fact at 3
  entries.
- The gate stops make at `test/gate.sh`. M9 and M13 fail an example or an
  eval row, thus the run does not reach their kickoff killers (the path
  regressions of the `out` type and of the unfold round trip).
- M11: the kickoff killer `codata-step.lang` does not kill it. Under the
  mutant, the def `nats` of that file fails first with the same
  `TYPE_MISMATCH`, thus the check row passes.
- M10: the run 1 edit `!f->cofamily` stopped at the compiler warning
  `-Wtautological-compare`, not at a guest test. The edit is now
  `f->cofamily && f->ctor_count > 4096u`, and the rerun killed it.
- Edit sites: M1 edits `out_field` (`counfold_at(m, u, u->args[1])`), not
  `counfold_at`, because the Pi chain path also calls `counfold_at`. M3
  puts `counfold_at(m, u, v)` in place of the `CLOS_OUT_PI` closure. M5
  and M6 add an `OP_COUNFOLD` test before the argument loop of the `VAL_OP`
  arm of the conversion: the conversion has no `unfold` arm of its own. M7
  adds `&& t->kind != CORE_PI`, which is false in the loop. M12 gives
  `put_text(o, "null")`.

## S5:  IO and the console build (2026-10-20)

### Chunk A:  domain and check

Chunk A adds the IO block to the domain and the IO case of the kit monad
rules. domain/domain.lang declares the layer family IOF (`ret`, `tau`,
`readLineOp`, `writeOp`, `argsOp`) and the cofamily IO after the Mealy
block (Q1 and Q2 (a)), and the defs `ioPure`, `readLine`, `write`, `args`,
`ioBind`, `ioMap` and `ioLoop` (Q3 to Q5 (a)). Each def is a guarded
unfold. `readLine`, `write` and `args` give one operation layer, then
`ret`; the seed `none` is the step before the operation. `ioBind` has the
state `Sum (IO A) (IO B)` and gives one `tau` at the switch to `f a`.
`ioLoop` has the state `IO (Sum R S)` and gives one `tau` for each round.
In check.c, `pure` at an `IO A` hint, and `map` and `bind` at an `IO A`
carried type, elaborate to the application of `ioPure`, `ioMap` or
`ioBind` (Q4 (a)). `bind` at IO with a function into another carrier is
TYPE_MISMATCH. Chunk A adds no Op, no Carrier and no switch arm.

| Suite | Count |
|---|---|
| examples | 31 checked, 0 failed |
| parse refusals | 14 |
| check refusals | 81 |
| evals | 151 |
| json builds | 31 |
| build refusals | 4 |
| regressions | 6 |
| printer regressions | 2 |
| path regressions | 179 checked, 0 failed |

`cc -fsyntax-only` (check-clang) passes. The dash sweep is empty.

Edit sites:

- domain/domain.lang: the IO block (31 lines) after `cofamily Mealy`.
- src/front/check.c: a forward declaration of `is_cofamily`; `io_of`,
  `io_type` and `io_def` before `carried`; `carried` and `carried_arg`
  accept an IO shape; the IO case at the start of `rule_pure`, and after
  `carried_arg` in `rule_map` and `rule_bind`; the seed of
  `rule_counfold` (D1).
- test/check: io-bind-carrier, io-pure-type and io-write-arg (each
  TYPE_MISMATCH), 3 lines in expect.txt.
- test/path-regressions.c: cases 174 to 179 (an IOF layer and an IO type,
  `pure` with refl of its layer, `map`, `bind`, the type of
  `out (write "a")`, the type of `out readLine`); D2.

Deviations:

- D1: `rule_counfold` (S4) infers the seed with `try_infer`. When the seed
  does not infer (`inl m`, `none`), its type comes from the binder of the
  step, as in `carried_arg`. The language has no annotation term, thus the
  `ioBind` seed `inl m` and the seed `none` of `readLine`, `write` and
  `args` need this fallback. Each S4 seed infers, thus no S4 result
  changes.
- D2: rule R4 makes the constructor name `ret` a core name. The S6b path
  regression `ua table at a Sigma carrier` declared a def `ret`;  it is now
  `retAll`. The name test (`is_reserved`, `name_free`, the builtin table)
  found no clash;  a search of all guest text found this one def.
- D3: the parameter order of `ioLoop` is S, then R
  (`ioLoop S R body s`), the order of its arrow type in the kickoff.
- D4: each rule finds its def by name at each use (`find_def`), as
  `equiv_sigma` finds `Equiv`. The rule finds the cofamily IO by the name
  of the family of the type. A domain with IO and without the def gives
  NAME_UNKNOWN.

Probe (before the domain edit, a langc in $TMPDIR with the IO block, a
trace of the layers):  `write "a"` gives a, ret;  `bind` of two writes
gives a, tau, b, ret;  `map` gives a, tau, ret;  `bind readLine` gives
read, tau, the line, ret;  `bind args` gives args, tau, the write, ret;
`ioLoop` of 2 rounds gives x, tau, tau, x, tau, tau, ret.

Mutants: chunk A has no mutant. M8 (`rule_bind` elaborates to `ioMap`)
waits for its killer examples/io.lang in chunk C.

#### Chunk A staged review (2026-10-20)

Reviewed all nine staged files. No CI or test weakening was found. Two
correctness findings were fixed:

- HIGH: `io_def` constructed unchecked helper applications. A replacement
  domain could declare `ioPure`, `ioMap` or `ioBind` with a `Nat` result,
  yet the corresponding surface expression checked as `IO Nat`. Helpers
  now require type parameters in `Type 0` and the expected specialized
  signature before their core applications are built.
- MEDIUM: an inferred IO mapper could return a type, accepting an invalid
  `IO (Type 0)` inside `out` and `fold`. Inferred helper type arguments
  are now rechecked at `Type 0` through the ordinary checker. Fresh scope
  names preserve neutral types and shadowed binders. Source printing uses
  the checker depth budget and distinguishes depth cuts from full buffers.

The review adds 18 C path regressions: incompatible and missing helpers,
compatible replacement helpers, closed and neutral universe violations,
neutral small types, shadowed type binders, nested `pure` inference and a
240-arrow inferred type. Eight new negative cases fail with the original
staged checker. `make check` passes, with 197 path regressions and no
failures. The same 197 cases pass with AddressSanitizer and
UndefinedBehaviorSanitizer. Validation ran serially in one isolated index
snapshot. Both findings are fixed; no blockers remain.

### Chunk B:  driver, console runtime and build

Chunk B adds the IO driver, the runtime of a console program and the
console build. src/runtime/driver.c `io_run` runs an IO value. Each `out`
spends 1 fuel. The budget EVAL_FUEL_STEPS starts again after each
operation (Q6 (a)). At `ret`, the status is 0. At `tau t`, the driver
continues with `t`. readLine gives the line without its newline, or
`none` at the end of the input. write writes the bytes, then flushes
them; a write error is IO_WRITE. args gives a JSON array of the
arguments after the program name. src/runtime/rt_main.c is the main of a
console program. It loads the embedded domain and program text, checks
the program, finds `main` and runs it. It exits 0 at `ret`, or 1 with
the diagnostic on stderr. src/build.c gives `langc build PROG -o OUT`
(Q7 and Q8 (a)). It refuses a program with no `main` (BUILD_MAIN) and a
`main` that does not convert with `IO Unit` (BUILD_MAIN_TYPE). Then
libtcc compiles the runtime units and one unit with the program text
into an executable. A libtcc error is BUILD_TCC. The kit `build` verb is
now `langc json PROG [-o OUT]` (Q9 (a), O20 (a)). test/terminal.c
(Q10 (a)) builds each test/term fixture with `langc build`, runs it and
compares the output and the exit code. A refusal fixture compares the
langc code. make check runs test/terminal.c last.

| Suite | Count |
|---|---|
| examples | 31 checked, 0 failed |
| parse refusals | 14 |
| check refusals | 81 |
| evals | 151 |
| json | 31 |
| json refusals | 4 |
| regressions | 6 |
| printer regressions | 2 |
| path regressions | 204 checked, 0 failed |
| terminal | 4 checked, 0 failed |

`cc -fsyntax-only` (check-clang) passes. It now also reads rt_main.c and
test/terminal.c. The dash sweep of the 30 chunk B paths is empty. The
terminal runner passes in the sandbox. It writes only under build/term/.

The USER commit 4350c48 (`Add console builds and terminal runtime (S5
chunk B)`) adds quotes to the two `$(TCC_HOME)` paths in the Makefile
(the `-I` flag and libtcc.a). The mutant runs used the staged tree
013f19eb, which has no quotes. `make -B check` on 4350c48 gives the
counts above.

Edit sites (30 paths):

- src/front/eval.c and core.h: `eval_out` and `eval_apply`, the exported
  wrappers of `reduce_out` and `apply_value`.
- src/runtime/driver.c and driver.h (new): `io_run`.
- src/runtime/rt_main.c (new): the main of a console program.
- src/build.c and build.h (new): the main type test and `console_build`.
- src/main.c: the `json` verb (was `build`) and the `build` verb, with
  `-o` required. src/json.h: the verb name in the comment.
- gen/embed.c: the `-r` mode. It writes build/runtime.c from the
  `tcc -E` output of each runtime unit (build/rt/, 12 units).
- Makefile: DRIVER, TCC_HOME, TCC_FLAGS and RUNTIME; the rules
  build/runtime.c, build/runtime.o and build/terminal; the link of langc
  with `$(CC)` and libtcc.a (D5); `$(TCC_FLAGS)`, rt_main.c and
  test/terminal.c in check-clang. make check gives CC, TCC and TCC_HOME
  to test/regressions.js and runs build/terminal last.
- test/terminal.c (new) and test/term/ (10 files): the run fixture hello
  (.lang, .args, .out and .code), and the refusal fixtures no-main
  (BUILD_MAIN), main-type (BUILD_MAIN_TYPE) and tcc-out (BUILD_TCC)
  (.lang and .code).
- test/path-regressions.c: the helpers `io_driver_run`,
  `io_driver_case`, `io_write_bytes` and `io_segment_budget`, the
  sources `io_echo` and `io_echo_loop`, and cases 198 to 204 (echo, end
  of input, a last line with no newline, args, the write bytes, a small
  budget for each segment, the tau loop trap).
- test/gate.sh, test/regressions.js and test/printer-regressions.js: the
  `json` verb, and the gate lines `json: N checked` and
  `json refusals: N checked` (D9). test/regressions.js: the
  `domainCompiler` link (D5).
- docs/host/README.md (5 sites), docs/host/CAPABILITY.md (2),
  domain/README.md (1) and formers/tcc-json.md (2): the `json` verb.

Deviations:

- D5: ~/.local/lib/libtcc.a holds Mach-O arm64 objects (clang built
  it), and the tcc linker reads only ELF objects. Thus `$(CC)` links
  build/langc. The `domainCompiler` of test/regressions.js linked
  src/main.c with tcc, without build.c and driver.c, but main.c now calls
  `console_build`. A new rule build/runtime.o (`$(CC) $(CFLAGS) -c`)
  gives one runtime object. langc and each domain compiler link it with
  `$CC`, src/build.c, src/runtime/driver.c and libtcc.a. The `build` verb
  of a domain compiler has the default domain in its runtime. The domain
  compiler tests use only `check` and `json`.
- D6: the precondition drift. The USER commit of chunk A is 0d002ec
  (tree 7f164f67), with the staged review block. Thus the base is 197
  path regressions, not 179. The chunk B targets are path regressions
  204 (197 + 7; the kickoff row B has 186) and terminal 4.
- D7: the refusal fixtures build into build/term/none/NAME. terminal.c
  does not make build/term/none/. Thus libtcc cannot write the output of
  tcc-out, and the code is BUILD_TCC (the libtcc text starts with
  `could not write`).
- D8: on macOS, a `fmemopen` stream in mode "r" accepts writes (fwrite
  gives 4, fflush gives 0). The write error regression uses /dev/null,
  opened for reading (fwrite gives 0).
- D9: the `json` rename also changes test/printer-regressions.js (not in
  the s2 list). The gate line `json: N builds checked` is now
  `json: N checked`. These stay as they are: the plan text of Q9 and O20
  in docs/PLAN.md, the history in docs/STATUS.md, formers/FORMERS.md:283
  (`langc build --js` of the tcc-js host) and design/DESIGN.md.

Observation (no fix): the EVAL_FUEL text says `more than 20000000
steps` at each budget. `io_run` at budget 290 gives the same text. Only
the path regressions use a budget other than EVAL_FUEL_STEPS. Chunk C
records this as a note or an O item.

Probes: `langc build` of `def main : IO Unit := write "hi\n"` gives an
executable that writes `hi\n` and exits 0. One echo round of `ioLoop`
costs at most 290 fuel: 290 is the smallest budget that passes, for 1
line and for 100 lines. The segment budget regression uses 1000.

Mutants (kickoff section 4). Each run did `git add` first and `make -B`
for each rebuild, then restored the file and examined `git diff`. After
the 11 runs, `make -B check` passes at the gate counts and the tree is
013f19eb.

| Id | File | Edit | First killer | Result |
|---|---|---|---|---|
| M1 | src/runtime/driver.c | readLine gives `some ""` at the end of the input | path regression end of input | KILLED |
| M2 | src/runtime/driver.c | readLine keeps the `\n` | path regression echo | KILLED |
| M3 | src/runtime/driver.c | write writes `len - 1` bytes | path regression echo | KILLED |
| M4 | src/runtime/driver.c | args starts at argv[0] | path regression args | KILLED |
| M5 | src/runtime/driver.c | no budget reset at an operation | path regression a small budget for each segment | KILLED |
| M6 | src/runtime/driver.c | `ret` gives status 1 | path regression echo (status 1) | KILLED |
| M7 | src/runtime/driver.c | stop at `tau` as at `ret` | path regression the `ioBind` effect order | chunk C |
| M8 | src/front/check.c | `rule_bind` elaborates to `ioMap` | gate.sh examples/io.lang | chunk C |
| M9 | src/build.c | TCC_OUTPUT_MEMORY in place of TCC_OUTPUT_EXE | terminal hello (exit -1) | KILLED |
| M10 | src/build.c | an empty program unit | terminal hello (exit 1) | KILLED |
| M11 | src/main.c | `json` runs CMD_CHECK | gate.sh json examples/algebra.lang | KILLED |
| M12 | src/build.c | the main type test accepts each type | terminal main-type | KILLED |
| M13 | src/runtime/rt_main.c | `io_run` gets argv + 2 | terminal hello (the output) | KILLED |

- M3: the first killer is the path regression echo. It runs before the
  write bytes case in main. The kickoff names write bytes.
- M12: main-type gives BUILD_TCC in place of BUILD_MAIN_TYPE. Its build
  goes on to libtcc, and build/term/none/ does not exist (D7).
- M13: the kickoff names the terminal row args (chunk C). The chunk B
  fixture hello reads args, and it kills M13.

### Chunk C:  examples, terminal tests, mutants and docs

Chunk C adds examples/io.lang, the IO evals, the io golden, 8 terminal
rows and 2 path regressions. examples/io.lang has echo (`ioLoop` of
`echoRound`), greet (`args`), count (`countLines` by `map` in
`countRound`, then `bind` to write the number) and twoWrites. The guest
helper `trace A n ins m` gives the JSON text of the layers of `m` (ret,
tau, read, args, the write text, and `...` when it reads n layers).
`result A n ins m` gives the value at `ret`. The 12 eval rows are
readLineTrace, readLineValue, endOfInput, writeTrace, argsTrace,
bindOrder, mapValue, echoTrace, echoCut, countValue, countTrace and
greetTrace. Each layer agrees with the domain defs: `ioBind` gives a
`tau` at the switch to `f a`, and `ioLoop` gives a `tau` at each round.
Thus echo of 2 lines is read, tau, p, tau, tau, read, tau, q, tau, tau,
read, tau, ret. The golden test/json/io.json holds the closed values
(noText, lines, the 8 trace texts and the 4 Option values). The IO defs
and the functions are not in it. The terminal rows are echo (`ab`, an
empty line, `cd`), end-of-input (no NAME.in, writes `none`), no-newline
(`ab\nxy` gives `ab\nxy\n`), args (3 arguments, one with a space and one
with quotes), empty-input (an empty NAME.in), two-writes (`a\nb\n`),
lines-1000 (`line 1` to `line 1000`, made one time with awk) and spin
(exit 1, spin.err). The path regressions 205 and 206 are `the ioBind
effect order` (a bind of two writes gives `ab`) and `ioLoop of 1000
rounds` (io_echo_loop, 1000 lines of `ab`, budget 300).

| Suite | Count |
|---|---|
| examples | 32 checked, 0 failed |
| parse refusals | 14 |
| check refusals | 81 |
| evals | 163 |
| json | 32 |
| json refusals | 4 |
| regressions | 6 |
| printer regressions | 2 |
| path regressions | 206 checked, 0 failed |
| terminal | 12 checked, 0 failed |

`cc -fsyntax-only` (check-clang) passes. The dash sweep of the chunk C
paths is empty. The terminal runner passes in the sandbox. It writes
only under build/term/.

Edit sites (40 paths):

- examples/io.lang (new): echo, greet, count, twoWrites and the guest
  helpers `trace` and `result`.
- test/eval/expect.txt: the 12 rows `io ...`.
- test/json/io.json (new): the golden of examples/io.lang.
- test/term/ (30 new files): the run fixtures echo, end-of-input,
  no-newline, args, empty-input, two-writes, lines-1000 and spin (.lang,
  .out and .code, and .in, .args or .err when the row needs it).
- test/terminal.c: an optional NAME.err (D10).
- test/path-regressions.c: the helper `io_loop_rounds` and the cases 205
  and 206.
- docs/PLAN.md: the section 6.1 note of Q2, the section 7 note of Q8, the
  row S5 in section 8, and O21, O22 and O23 in section 9.
- formers/FORMERS.md: the IO case of F5 and of F7. formers/tcc-json.md:
  the S5 shapes. README.md: the console programs. docs/STATUS.md: this
  block.

Deviations:

- D10: terminal.c reads an optional NAME.err. The standard error of the
  run must start with its text. The s3 prompt asked for EVAL_FUEL on
  stderr, and run_fixture did not read stderr.
- D11: spin gives OOM, not EVAL_FUEL. spin is
  `unfold (fun (u : Unit) => tau Unit Unit u) unit`. Its standard error
  is `langc: OOM: main: the arena limit is reached`: the arena fills in
  about 1 s, before 20000000 steps. spin.err holds the full diagnostic line.
  The USER ruling Q-C1 (a) keeps this with no code change, and O22 has
  the note (a reclaim at each `tau` can give EVAL_FUEL).
- D12: the S4 ruling names the open item O21, but section 9 did not have
  it. Chunk C adds O21 before O22. The EVAL_FUEL text of the chunk B
  observation is the new item O23, not a part of O22.
- D13: test/term has 30 new files, not 32. end-of-input and two-writes
  have no NAME.in. Thus chunk C part 1 has 35 paths, not 37.

Mutants (kickoff section 4). The run did `git add` of the 35 chunk C
paths first (staged tree 0515e274) and `make -B` for each rebuild. After
each run, it restored the file and examined `git diff`. After the 2
runs, `make -B check` passes at the gate counts and the tree is 0515e274.

| Id | File | Edit | First killer | Result |
|---|---|---|---|---|
| M7 | src/runtime/driver.c | stop at `tau` as at `ret` | path regression echo (status 0, 0 bytes) | KILLED |
| M8 | src/front/check.c | `rule_bind` elaborates to `ioMap` | gate.sh eval io bindOrder | KILLED |

- M7: io_echo is `bind readLine ...`, thus a `tau` comes before its
  write. The chunk B case echo runs before the new cases. 8 of the 206
  path regressions fail, `the ioBind effect order` and `ioLoop of 1000
  rounds` among them.
- M8: the mutant elaborates `bind t f` at IO to `ioMap A (IO B) f t` and
  keeps the type `IO B`. examples/io.lang checks. bindOrder gives
  `["a","tau","ret"]` in place of `["a","tau","b","ret"]`. gate.sh has 6
  failures: bindOrder, echoTrace, echoCut, countTrace, greetTrace and
  json examples/io.lang.

S5 close: chunks A, B and C are in, with the rulings Q1 to Q12 = (a) and
Q-C1 = (a). 13 of 13 mutants are KILLED (M1 to M6 and M9 to M13 in chunk
B, M7 and M8 in chunk C). The final gate is examples 32, parse refusals
14, check refusals 81, evals 163, json 32, json refusals 4, `gate: 0
failures`, regressions 6, printer regressions 2, path regressions 206
and terminal 12. The next slice is S6 (the filesystem operations 2 to
10, docs/PLAN.md section 8).

## S6a:  the filesystem read operations (2026-10-20)

### Chunk A:  layer and check

Chunk A adds the layer of the read operations 2, 4, 6, 8 and 10 (S6 Q1,
Q2 and Q15 (a)). domain/domain.lang adds 5 constructors to IOF after
`argsOp`, in the operation order:  `listDirOp`, `findOp`, `readFileOp`,
`searchOp` and `diskUsageOp` (12 fields). It adds the defs `listDir`,
`find`, `readFile`, `search` and `diskUsage` in the form of `args`, and
the 5 arms in each leg of `ioBind` and in `ioLoop`. Each fold over IOF
now takes 10 functions, then the target. examples/io.lang adds the 5 arms
in `layers` and `result`. Each arm gives a fixed `inr` response:  `[]`
for listDir, find and search, `{}` for diskUsage and `"text"` for
readFile. The 5 trace defs bind the operation into a `write` of the
response. driver.c does not change (chunk B).

| Suite | Count |
|---|---|
| examples | 32 checked, 0 failed |
| parse refusals | 14 |
| check refusals | 84 |
| evals | 168 |
| json | 32 |
| json refusals | 4 |
| gate | 0 failures |
| regressions | 6 |
| printer regressions | 2 |
| path regressions | 206 checked, 0 failed |
| terminal | 12 checked, 0 failed |

`cc -fsyntax-only` (check-clang) passes. The dash sweep is empty.

Edit sites:

- domain/domain.lang:  the IOF line, its comment, the IO monad comment,
  the 5 defs after `args`, `ioBind` (2 legs) and `ioLoop`.
- examples/io.lang:  the comment, `layers`, `result`, `showJson`,
  `showText` and the 5 trace defs. test/eval/expect.txt:  5 rows.
  test/json/io.json:  the 5 trace texts.
- test/check:  io-listdir-nat.lang, io-find-arity.lang and
  io-readfile-bind.lang, each TYPE_MISMATCH.
- test/path-regressions.c:  the 6 guest folds over IOF and the budget of
  `ioLoop of 1000 rounds` (D2, D3).
- docs/PLAN.md:  `### USER ruling (S6)` and the section 8 rows S6a and
  S6b.

| Id | Deviation |
|---|---|
| D1 | The prompt names 10 field names. The 5 rows have 12 (2, 3, 2, 3, 2). The name probe tested all 22 names (5 defs, 5 constructors, 12 fields):  `langc check` accepts a def of each name, and the control `def args` gives REFUSE_NAME (rule R4). No name is `Bool` (RESERVED). |
| D2 | Kickoff 1.1 counts 5 fold sites over IOF. test/path-regressions.c has 6 more guest folds over `out` (lines 123 and 1212 to 1220) with the 5 S5 arms. After the layer edit, the 3 accept cases failed at the fold arity, and the 3 refusal cases (1212, 1214, 1216) gave TYPE_MISMATCH for the fold arity, not for the universe. Each fold now has the 5 new arms. Each refusal again gives `expected a term of type Type 0, found a term of type Type 1` (`langc check` by hand). |
| D3 | The path regression `ioLoop of 1000 rounds` (budget 300) stopped at EVAL_FUEL in the first round. A budget probe gives the minimum 330 (290 at S5:  8 fuel for each new constructor in the folds of one round). The budget is now 340 (the S5 margin of 10). The 4 constructors of S6b add about 32:  S6b chunk A sets the budget again. |
| D4 | The USER gave no letter for S6. The PLAN block records each Q as the default (a) (prompt section 0). |

| Id | Mutant | Killer |
|---|---|---|
| M1 | ioBind leg `inl`:  the searchOp arm gives `q p` | gate.sh `FAIL eval io searchTrace` (KILLED) |
| M2 | `find`:  `findOp ... g r` | gate.sh `FAIL eval io findTrace` (KILLED) |

Each mutant ran after `git add`, with `make -B check`, and was restored
from the index. M3 to M7 run in chunk C.

### Chunk B:  driver and path regressions

Chunk B adds the C side of the read operations 2, 4, 6, 8 and 10 (S6
Q7). driver.h adds `TrashForm` and `IoEnv` (budget, in, out, argc, argv,
trash_root, trash_form, force_exdev). `io_run` takes the env. driver.c
adds the Json makers (`jnum`, `jobj`, `jfNil`, `jfCons`), the key and
kind tables (made once in `io_run_init`), 5 Layer values with their
`io_step` cases and the 5 operations:  `list_dir`, `find_paths`,
`read_file`, `search_text` and `disk_usage`. The walk uses `lstat` and
follows no link. listDir and find sort the names with `strcmp`. find
matches the base name with `fnmatch` (flags 0). search skips a file with
a NUL byte in its first 8 KiB, splits each file at `\n` and compares
bytes. diskUsage gives `total`, `free` and `available` from `statvfs`
(each block count times `f_frsize`). A value above INT64_MAX is decimal
text. rt_main.c sets the trash root from HOME (macOS:  `HOME/.Trash`;
else `XDG_DATA_HOME/Trash` or `HOME/.local/share/Trash`).
test/path-regressions.c adds the tree helpers and 33 read rows over one
tree, build/fs/pr/read.

| Suite | Count |
|---|---|
| examples | 32 checked, 0 failed |
| parse refusals | 14 |
| check refusals | 84 |
| evals | 168 |
| json | 32 |
| json refusals | 4 |
| gate | 0 failures |
| regressions | 6 |
| printer regressions | 2 |
| path regressions | 239 checked, 0 failed |
| terminal | 12 checked, 0 failed |

`cc -fsyntax-only` (check-clang) passes. The dash sweep is empty.

Edit sites:

- src/runtime/driver.h:  `TrashForm`, `IoEnv` and the `io_run`
  prototype.
- src/runtime/driver.c:  `IoNames`, the key and kind tables, `IoRun`,
  the 5 Layer values and their `io_step` cases, `fs_text`, `fs_fail`,
  `fs_root_stat`, `fs_walk`, `fs_entry`, `fs_scan`, `fs_read`, the 5 operations and
  `json_num`.
- src/runtime/rt_main.c:  `trash_root` and the env.
- test/path-regressions.c:  `io_driver_exec`, `io_driver_run`,
  `fs_reset`, `FsForm`, `FsItem`, `fs_item`, `fs_tree`, `fs_read_items`,
  `fs_late`, `fs_source`, `fs_case`, `fs_near`, `fs_disk_usage`,
  `fs_three_ops` and `fs_read_rows` (called last in main).

| Id | Deviation |
|---|---|
| D5 | The kickoff gives no text for 3 refusals. Chunk B uses fixed texts:  `NAME: the path holds a NUL byte` (no path:  the path holds the NUL byte), `find: ROOT: the glob holds a NUL byte` and `search: ROOT: the search text is empty`. |
| D6 | A walk error below the root names the path that failed, not the root. |
| D7 | When `opendir` of a search root gives ENOTDIR, search reads the root as one file (row `search file root`). A missing root keeps ENOENT (row `search missing root`). |
| D8 | diskUsage of a file path gives the object, not `inl`:  `statvfs` takes a file (row `diskUsage file path`). |
| D9 | A full read of driver.c found 1 defect. In `search_text` (line 588), the ENOTDIR branch of a file root reset `failed` after an OOM in `fs_fail`, and the run continued with a diagnostic set. The branch now also needs `response != NULL`. Each other NULL response has an OOM diagnostic (`new_value` in eval.c calls `oom`;  `arena_text` and `hit_push` call `diag_fail`). |
| D10 | jsonRender (src/front/jsontext.c `escape_byte`) writes `\u00XX` for each byte below 0x20. Thus the goldens hold `\u000a` for `\n`, `\u000d` for `\r` and `\u0000` for NUL. |
| D11 | The budget probe (a runner script under $TMPDIR;  probe.c includes path-regressions.c with `main` renamed):  `fs_three_ops` (diskUsage, readFile, listDir, then `write "ok"`) fits in one segment at budget 271. At 270 it gives status 1 and EVAL_FUEL. The row uses 281 (`FS_SEGMENT_BUDGET`, the minimum + 10). |
| D12 | `io_driver_run` is now 2 parts:  `io_driver_exec` (the status, the output bytes and the Diag) and the compare. `io_driver_exec` makes and releases its own arena. The Diag holds `def` and `msg` inline, thus it stays valid after the release. The rows `diskUsage values` and `diskUsage file path` use `io_driver_exec` and parse the output. |
| D13 | A failure of `fs_reset` or `fs_tree` adds 1 failure to the count. The 33 rows run after it and fail at their own compare. |

Chunk B runs no mutants. M3 to M7 (driver.c) run in chunk C. Their
killers are the rows `listDir sorted`, `find link not walked`, `readFile
NUL`, `search binary` and `diskUsage values`.

The staged review found that a link supplied as the root was followed by
listDir, find and search, against S6 Q11. The walk now inspects the root
with lstat before opendir, including a root with trailing slashes. Search
skips a link root and reads a regular file root directly. This replaces
the ENOTDIR fallback described in D7 and D9. Four new rows failed before
the fix. Eight added rows cover root links, trailing slashes, a dangling
link and readFile following a link. After the fix, `make check` passes,
including 239 path regressions and 12 terminal rows.

### Chunk C:  examples, terminal rows, mutants and docs

Chunk C adds examples/fs.lang, 5 fs terminal rows and the mutants M3 to
M7. driver.c does not change. examples/fs.lang has closed functions over
sample responses in the form that the driver gives:  `itemsOf`,
`fieldOf`, `textOf` and `numberOf` (block folds over Json), then `names`
(listDir), `paths` (find), `hitTexts` (search) and `total` (diskUsage).
No IO runs:  the gate evaluates 4 defs. test/terminal.c adds the tables
`fs_rows` and `fs_items` and the helper `fs_tree`. Each fs row gets its
own tree under build/fs/term/NAME before the spawn. The runner removes
build/fs/term at the start. The rows list-dir, find-glob, read-file,
search-text and disk-usage each write the response (disk-usage writes
the key names only).

| Suite | Count |
|---|---|
| examples | 33 checked, 0 failed |
| parse refusals | 14 |
| check refusals | 84 |
| evals | 172 |
| json | 33 |
| json refusals | 4 |
| gate | 0 failures |
| regressions | 6 |
| printer regressions | 2 |
| path regressions | 239 checked, 0 failed |
| terminal | 17 checked, 0 failed |

Edit sites:

- examples/fs.lang (new), test/json/fs.json (new), test/eval/expect.txt
  (4 rows:  `listNames`, `foundPaths`, `hitLines`, `usageTotal`).
- test/terminal.c:  the header comment, `FsKind`, `FsItem`, `fs_rows`,
  `fs_items`, `fs_tree`, and in `main` the build/fs/term cleanup and the
  `fs_tree` call before `run_fixture`.
- test/term:  list-dir, find-glob, read-file, search-text and disk-usage
  (each `.lang`, `.code` and `.out`).
- Docs:  docs/PLAN.md (the section 6.2 S6a note, the section 8 row S6a),
  formers/FORMERS.md (F7 `IO A` and the matrix row F7), formers/tcc-json.md
  (`## S6 shapes`) and README.md (the S6a paragraph).

| Id | Deviation |
|---|---|
| D14 | HEAD is 638f37c, the USER commit of chunk B with the review fix (tree 54b7173d, not the staged tree 266705ee). The path regressions are 239, not 231. The chunk C gate target uses 239. The mutant sites moved:  M3 line 388, M4 line 364, M5 line 517, M6 line 572, M7 line 648. |
| D15 | M5 uses `strnlen (bytes, len)` in place of `strlen (bytes)`. The `fs_read` buffer has no NUL after the bytes, thus `strlen` reads past the buffer for a file with no NUL byte. At the row the effect is the same:  the length stops at the first NUL byte. |
| D16 | The first killer of M4 is `listDir sorted` (with `stat` the link `beta` gets the kind `file`), not `find link not walked`. listDir and find use the same `fs_entry`. `find link not walked` also fails under M4 (5 rows fail). |
| D17 | The find-glob tree holds a link `link` to `sub`:  the output has no path under `link`. The search-text tree holds `bin.dat` with a NUL byte:  the output has no hit in it. |
| D18 | The docs session started on a3b31e0, the USER commit of the chunk C code (tree 2590793a, the staged tree of session 1), not on 638f37c. The docs are a separate change on a3b31e0 (USER ruling 2026-10-21). |

| Id | Mutant (driver.c) | First killer | Result |
|---|---|---|---|
| M3 | `fs_walk`:  the skip of `.` and `..` only when `deep` is not 0 | path regression `listDir sorted` (3 rows fail) | KILLED |
| M4 | `fs_entry`:  `stat` in place of `lstat` | path regression `listDir sorted` (D16;  5 rows fail) | KILLED |
| M5 | `read_file`:  the Text length from `strnlen` (D15) | path regression `readFile NUL` (1 row fails) | KILLED |
| M6 | `search_text`:  no 8 KiB NUL test (`1 \|\| memchr`) | path regression `search binary` (1 row fails) | KILLED |
| M7 | `disk_usage`:  `total` from `f_bsize` in place of `f_frsize` | path regression `diskUsage values` (2 rows fail) | KILLED |

Each mutant ran after `git add`, with `make -B check`, and was restored
with `git checkout -- src/runtime/driver.c`. `git diff --quiet` passed
after each restore. Under M7 the total was 126562507685888 (256 times
the real total:  `f_bsize` 1048576, `f_frsize` 4096).

7 of 7 S6a mutants KILLED.

## S6b:  the filesystem write operations (2026-10-21)

### Chunk A:  layer and check

Chunk A adds the layer of the write operations 3, 5, 7 and 9 (S6 Q1,
Q2 and Q15 (a)). domain/domain.lang adds 4 constructors to IOF after
`diskUsageOp`, in the operation order:  `makeDirOp`, `moveOp`, `copyOp`
and `trashOp` (10 fields). Thus each S6a fold arm keeps its place. It
adds the defs `makeDir`, `move`, `copy` and `trash` (each at
`Sum Text Unit`) in the form of `args` after `diskUsage`, and the 4 arms
in each leg of `ioBind` and in `ioLoop`. Each fold over IOF now takes 14
functions, then the target. examples/io.lang adds the 4 arms in `layers`
and `result`. Each arm gives the fixed response `inr unit`. The 4 trace
defs bind the operation into a `write` of the response (`showUnit` gives
`unit`). driver.c does not change (chunk B).

| Suite | Count |
|---|---|
| examples | 33 checked, 0 failed |
| parse refusals | 14 |
| check refusals | 86 |
| evals | 176 |
| json | 33 |
| json refusals | 4 |
| gate | 0 failures |
| regressions | 6 |
| printer regressions | 2 |
| path regressions | 239 checked, 0 failed |
| terminal | 17 checked, 0 failed |

`cc -fsyntax-only` (check-clang) passes. The dash sweep is empty.

The name probe tested the 18 names (4 defs, 4 constructors, 10 fields)
against `RESERVED` (`{"Bool"}`), the domain, the quoted core names in
src/front and the guest defs in examples/ and test/:  no hit.

Edit sites:

- domain/domain.lang:  the IOF line, its comment, the IO monad comment,
  the 4 defs after `diskUsage`, `ioBind` (2 legs) and `ioLoop`.
- examples/io.lang:  the comment, `layers`, `result`, `showUnit` and the
  4 trace defs. test/eval/expect.txt:  4 rows. test/json/io.json:  the 4
  trace texts (the S6a text is an exact prefix of the new text).
- test/check:  io-move-nat.lang (`expected a term of type Text, found a
  term of type Nat`) and io-makedir-bind.lang (`the binder type Json
  differs from the expected type Sum Text Unit`), each TYPE_MISMATCH.
- test/path-regressions.c:  the 4 new arms in each of the 6 guest folds
  over `out`, the budget of `ioLoop of 1000 rounds` and
  `FS_SEGMENT_BUDGET` (D2, D3). The 3 refusal cases of the guest folds
  again give `expected a term of type Type 0, found a term of type
  Type 1`.

Budgets (one runner script under $TMPDIR each;  the probe binary reads
the budget from the environment and bisects):

| Row | S6a minimum | S6b A minimum | Budget |
|---|---|---|---|
| `ioLoop of 1000 rounds` | 330 | 362 (361 gives EVAL_FUEL) | 372 |
| `layer 3 operations at one segment` | 271 | 295 (294 gives EVAL_FUEL) | 305 |

| Id | Deviation |
|---|---|
| D1 | Precondition:  HEAD 71b0895 (USER, on a3b31e0) has the tree f838339c, not the staged tree fc0246e4. The USER changed README.md, docs/PLAN.md and formers/tcc-json.md (the error text forms of the input validation) before the commit. The difference is in docs only, thus the build went on. |
| D2 | The prompt names only the budget of `ioLoop of 1000 rounds`. The 4 new constructors also make `layer 3 operations at one segment` (`FS_SEGMENT_BUDGET` 281, S6a B D11) stop at EVAL_FUEL. A probe of the same form gives the minimum 295 (S6a 271 + 24:  3 operations through `ioBind`). The budget is now 305 (the minimum + 10). |
| D3 | The `ioLoop` minimum is 362 (S6a 330 + 32:  8 fuel for each new constructor in the folds of one round, as kickoff 1.6 gives). The budget is now 372 (the minimum + 10). |
| D4 | The response text of a write operation in the traces is `unit` (`showUnit`). The kickoff and the prompt name no text. |

| Id | Mutant | Killer |
|---|---|---|
| M8 | `copy`:  `copyOp ... t f` (from and to swapped) | gate.sh `FAIL eval io copyTrace` (KILLED) |

M8 ran after `git add`, with `make -B check`, and was restored with
`git checkout -- domain/domain.lang` (`git diff --quiet` gives 0). M9 to
M13 run in S6b chunk C (driver mutants).

### Chunk B:  driver and path regressions

Chunk B adds the driver of the write operations 3, 5, 7 and 9 (kickoff
1.2, S6 Q6 and Q7 (a)) and 23 path regressions. Each response is
`inr unit`, or `inl` of the text `OP: PATH: TEXT`. The guest layer does
not change (chunk A), thus the budgets do not change.

| Suite | Count |
|---|---|
| examples | 33 checked, 0 failed |
| parse refusals | 14 |
| check refusals | 86 |
| evals | 176 |
| json | 33 |
| json refusals | 4 |
| gate | 0 failures |
| regressions | 6 |
| printer regressions | 2 |
| path regressions | 262 checked, 0 failed |
| terminal | 17 checked, 0 failed |

`cc -fsyntax-only` (check-clang) passes. `git diff --check` and the dash
sweep are empty. The budgets of `ioLoop of 1000 rounds` (372) and
`FS_SEGMENT_BUDGET` (305) pass with no change.

Edit sites:

- src/runtime/driver.c:  `#include <time.h>`;  `IoNames` (`make_dir`,
  `move`, `copy`, `trash`);  `Layer` (`LAYER_MAKE_DIR`, `LAYER_MOVE`,
  `LAYER_COPY`, `LAYER_TRASH`);  `io_names` (the field counts 2, 3, 3
  and 2 of `makeDirOp`, `moveOp`, `copyOp` and `trashOp`;  the
  diagnostic text says `S6b shape`);  `layer_of`;  4 `io_step` cases
  (`-Wswitch-enum`). The new functions between `disk_usage` and
  `io_run_init`:  `fs_unit`, `FsFault`, `fs_fault` (it keeps the first
  failure), `fs_fault_value`, `fs_join`, `fs_write_all`, `make_dir`,
  `fs_copy_file` (O_EXCL, then `fchmod (mode & 07777)`), `fs_copy_dir`
  (mkdir 0700, the entries, then `chmod`), `fs_copy_link`, `fs_copy`,
  `fs_inside` (the realpath of the parent of TO, the last slashes cut),
  `fs_copy_top`, `fs_remove`, `fs_two_paths`, `fs_there`, `move_path`,
  `copy_path`, `trash_name`, `trash_dir`, `trash_info`, `trash_macos`,
  `trash_xdg` and `trash_path`. driver.h, rt_main.c and the `io_run`
  callers do not change (IoEnv already has `trash_root`, `trash_form`
  and `force_exdev`).
- test/path-regressions.c:  the file statics `pr_trash_root`
  (`PR_TRASH_ROOT`, build/fs/pr/trash), `pr_trash_form` and
  `pr_force_exdev` that `io_driver_exec` gives to IoEnv (a row sets
  them, then sets the defaults back);  `fs_unit_source` and
  `fs_write_case`;  the tree checks `fs_is`, `fs_gone`, `fs_bytes`,
  `fs_mode`, `fs_target` and `fs_trash_info` (each prints a FAIL line
  and adds no case);  `FS_W`, `FS_COUNT` and 13 item tables;
  `fs_write_rows` after `fs_read_rows` in `main`.

The 23 rows (R is the row root under build/fs/pr/;  each row makes its
own tree after the read rows reset build/fs/pr/):

| Row | Tree | Response | Tree checks |
|---|---|---|---|
| `makeDir parents` | empty | `unit` | R/a/b/c is a directory |
| `makeDir there` | d | `unit` | R/d is a directory |
| `makeDir file there` | f | `inl makeDir: R/f: File exists` | the bytes of R/f |
| `makeDir parent file` | f | `inl makeDir: R/f/sub: Not a directory` | the bytes of R/f |
| `move file` | a.txt | `unit` | a.txt gone, the bytes at b.txt |
| `move directory` | d, d/f.txt | `unit` | d gone, e/f.txt |
| `move destination there` | a.txt, b.txt | `inl move: R/b.txt: File exists` | the bytes of both |
| `move missing source` | empty | `inl move: R/none: No such file or directory` | b.txt not made |
| `move EXDEV` (`pr_force_exdev` 1) | d, d/f.txt (0640), d/l to f.txt | `unit` | d gone, e mode 0755, e/f.txt bytes and mode 0640, e/l to f.txt |
| `copy mode` | a.sh (0777) | `unit` | b.sh mode 0777 under umask 022, the bytes of a.sh and b.sh |
| `copy tree link` | d, d/f.txt, d/sub, d/sub/g.txt, d/l to f.txt | `unit` | e/f.txt, e/sub, e/sub/g.txt, e/l a link to f.txt, d/l and d/f.txt stay |
| `copy destination there` | a.txt, b.txt | `inl copy: R/b.txt: File exists` | the bytes of both |
| `copy into own tree` | d, d/f.txt | `inl copy: R/d/sub: the destination is in the source tree` | d/sub not made |
| `copy missing source` | empty | `inl copy: R/none: No such file or directory` | b.txt not made |
| `trash macOS form` (root R/home/.Trash) | a.txt, home | `unit` | a.txt gone, .Trash/a.txt, .Trash mode 0700 |
| `trash collision` (root R/home/.Trash) | a.txt, home/.Trash/a.txt | `unit` | a.txt gone, `.Trash/a 2.txt` new bytes, `.Trash/a.txt` old bytes |
| `trash XDG form` (TRASH_XDG, root R/data/Trash) | a.txt, data | `unit` | a.txt gone, files/a.txt, info/a.txt.trashinfo (D8) |
| `trash missing path` (root R/home/.Trash) | home | `inl trash: R/none: No such file or directory` | R/home/.Trash not made |
| `layer makeDir copy listDir` | src.txt | `[{"name":"dst.txt","kind":"file"}]` | the bytes of new/sub/dst.txt |
| `trash XDG missing parents` (root R/home/.local/share/Trash) | a.txt, home | `unit` | missing parents made, a.txt gone, files/a.txt bytes and info/a.txt.trashinfo |
| `copy restrictive umask` (umask 0777) | d, d/f.txt (0640), d/l to f.txt | `unit` | e mode 0755, e/f.txt bytes and mode 0640, e/l readable and pointing to f.txt |
| `trash restrictive umask` (umask 0777, root R/home/.Trash) | a.txt, home | `unit` | a.txt gone, .Trash mode 0700, .Trash/a.txt bytes |
| `trash XDG restrictive umask` (umask 0777, root R/home/.local/share/Trash) | a.txt, home | `unit` | missing parents made, Trash, files and info mode 0700, note mode 0600, file bytes and note contents, source gone |

The labels `makeDir parents`, `move file`, `move destination there`,
`copy mode` and `trash macOS form` are the killer rows of kickoff
section 4 (M9 to M13 run in chunk C).

| Id | Deviation |
|---|---|
| D1 | makeDir names the path as given:  a file at the path gives `File exists`, a file at a parent gives `Not a directory`, and an empty path gives ENOENT. The rows `makeDir file there` and `makeDir parent file` test the first two. No row tests the empty path. |
| D2 | move and copy do `lstat (TO)` first:  a TO that is there gives `OP: TO: File exists` (`rename` replaces a file and an empty directory with no error, kickoff section 5). A rename error names FROM. The rows `move destination there`, `copy destination there` and `move missing source` test this. |
| D3 | The EXDEV leg of move (or `force_exdev`) runs `fs_copy_top`, then `fs_remove (FROM)`. A failed copy keeps the source and the partial destination:  the driver does not delete a path that it did not surely make. The row `move EXDEV` tests the leg that succeeds only. |
| D4 | `fs_copy_top` also guards the EXDEV leg of move:  without the in-tree test, a move of `d` to `d/sub` copies with no end. The row `copy into own tree` tests the copy. No row tests the move. |
| D5 | Fixed texts (not `strerror`):  `copy: TO: the destination is in the source tree`;  `OP: PATH: copy cannot copy this kind (not a file, a directory or a link)`;  `trash: PATH: there is no trash directory (no HOME)`. Only the first has a row. |
| D6 | trash does `lstat (PATH)` first:  a missing path gives `trash: PATH: No such file or directory` before the driver makes the root (the row `trash missing path` checks that the root is not there). The name is the base name with the last slashes cut. At a collision, ` N` goes before the last extension (a leading dot is not an extension):  the row `trash collision`. |
| D7 | The XDG form uses the same names. It checks `files/NAME` with lstat, makes `info/NAME.trashinfo` with O_EXCL (at EEXIST, the next name), then renames. When the rename fails, it removes only the info file. The info text:  `[Trash Info]\nPath=`, the percent encoded absolute path (the cwd, `/` and a relative path;  `A-Z a-z 0-9 - . _ ~ /` are kept), then `\nDeletionDate=YYYY-MM-DDThh:mm:ss\n` (the local time). |
| D8 | The row `trash XDG form` encodes the cwd as the driver does. The prompt gives the raw getcwd text. The two texts are the same when the cwd holds only kept bytes. The encode keeps the row correct in a checkout path with a space. The row checks the 19 date bytes by their shape. |
| D9 | The rows have more tree checks than the prompt names:  `move EXDEV` checks the mode 0755 of the directory (the `chmod` at the end of `fs_copy_dir`), `copy mode` checks that the source stays, and `layer makeDir copy listDir` checks the bytes of dst.txt. |

Review fixes add four rows to the 19 kickoff rows. The staged driver
passed the original 258 path regressions, then failed all four new rows.
XDG trash now makes missing parent directories, so a fresh home without
`.local/share` works. Newly created trash directories have mode 0700
and XDG info files have mode 0600 even under umask 0777. Existing
directory modes stay unchanged. Copy makes its temporary directory
usable before copying children, then restores the source mode. On macOS
it also preserves symlink modes without following the links: umask can
otherwise make a copied link unreadable. The copy row checks a file,
directory and symlink under that mask. All four rows pass after the fixes.

### Chunk C:  examples, terminal rows, mutants and docs

Chunk C adds 3 evals to examples/fs.lang, 4 fs terminal rows and the
mutants M9 to M13. driver.c does not change. examples/fs.lang adds
`faultOf` (the text of an `inl` response, `""` at `inr unit`) and
`faults` (the texts of the `inl` responses of a list, in the list order)
at the sample responses `made`, `refused` and `steps`. test/terminal.c
adds the rows make-dir, move-refuse, copy-tree and trash-home. Each
guest program writes the response, then the listDir response that shows
the tree after the operation (move-refuse also writes the bytes of
b.txt). trash-home runs with a copy of the environment that has
HOME=CWD/build/fs/term/trash-home/home (`fixture_env`):  the trash goes
into that HOME/.Trash, never the real HOME. terminal.c does not call
`setenv`.

| Suite | Count |
|---|---|
| examples | 33 checked, 0 failed |
| parse refusals | 14 |
| check refusals | 86 |
| evals | 179 |
| json | 33 |
| json refusals | 4 |
| gate | 0 failures |
| regressions | 6 |
| printer regressions | 2 |
| path regressions | 262 checked, 0 failed (D1) |
| terminal | 21 checked, 0 failed |

`cc -fsyntax-only` (check-clang) passes. `git diff --check` and the dash
sweep are empty.

Edit sites:

- examples/fs.lang:  the S6b comment, `noStep`, `faultOf`, `faults`,
  `made`, `refused`, `steps`, `madeFault`, `refusedFault` and
  `stepFaults`. test/eval/expect.txt:  3 rows. test/json/fs.json:  7 new
  instances after the 11 instances of S6a, which do not change.
- test/terminal.c:  the header comment, `MAX_ENV`, `run` (the parameter
  ENVP;  the other callers give `environ`), `fs_rows` (9 rows), 8
  `fs_items` lines, `fixture_env`, and in `run_fixture` the environment
  of the run.
- test/term:  make-dir, move-refuse, copy-tree and trash-home (each
  `.lang`, `.code` and `.out`).
- Docs:  docs/PLAN.md (the row S6b, the S6b note after O16) and
  README.md (the S6b paragraph).

| Eval | Value |
|---|---|
| `madeFault` (`faultOf` at `inr unit`) | `""` |
| `refusedFault` | `"move: d/b.txt: File exists"` |
| `stepFaults` (3 responses, one `inl`) | `cons "trash: d/none: No such file or directory" nil` |

| Row | Tree | Output |
|---|---|---|
| make-dir | empty | `unit`, then `[{"name":"b","kind":"dir"}]` (makeDir of a/b made a) |
| move-refuse | a.txt, b.txt | `move: build/fs/term/move-refuse/b.txt: File exists`, the bytes `b` of b.txt, then a.txt and b.txt |
| copy-tree | d, d/f.txt, d/sub, d/l to f.txt | `unit`, e holds f.txt (file), l (link) and sub (dir), then the tree holds d and e |
| trash-home | a.txt, home | `unit`, the tree holds home only, then HOME/.Trash holds a.txt |

| Id | Mutant (driver.c) | First killer | Result |
|---|---|---|---|
| M9 | `move_path`:  `0 &&` before the `fs_there (TO)` test (line 902;  no lstat, the rename replaces TO) | path regression `move destination there` (1 row fails) | KILLED |
| M10 | `fs_copy_file`:  `((void)mode, 1)` in place of the `fchmod` (line 766;  D2) | path regression `move EXDEV` (D2;  3 rows fail, `copy mode` among them) | KILLED |
| M11 | `trash_macos`:  `unlink (path)` in place of `rename (path, dest)` (line 1003) | path regression `trash macOS form` (3 rows fail) | KILLED |
| M12 | `make_dir`:  `if (i < len) continue;` (one `mkdir` of the full path, line 738;  D3) | path regression `makeDir parents` (2 rows fail) | KILLED |
| M13 | `layer_of`:  moveOp gives `LAYER_COPY` (line 155) | path regression `move file`:  a.txt stays (5 rows fail) | KILLED |

Each mutant ran after `git add`, with `make -B check`, and was restored
with `git checkout -- src/runtime/driver.c`. `git diff --quiet` passed
after each restore. 5 of 5 S6b driver mutants KILLED (M8 is KILLED in S6b chunk A).

The USER live trash probe (agent-lang-work/S6b-trash-probe.sh, outside
the sandbox, at the real HOME):  PASS. The USER ran it on 2026-10-21 at
13:18, in the S6 close session. The response is `unit` and the probe rc
is 0. `ls -l` of the source gives `No such file or directory`.
`ls -l ~/.Trash` shows one probe entry,
`agent-lang-trash-probe-20261021131849.txt` (32 bytes, the probe text).

| Id | Deviation |
|---|---|
| D1 | HEAD is 23aadaa, the USER commit of chunk B with review fixes (tree ccd269b2, not the staged tree 43e5172b). driver.c:  `trash_dir` makes the missing parents with mode 0700, `fs_copy_dir` and `fs_copy_link` keep usable modes under umask 0777, and the XDG info file gets mode 0600. path-regressions.c:  4 rows. The path regressions are 262, not 258. The USER ruled (2026-10-21) to use HEAD as the base. The mutant lines are on HEAD. |
| D2 | M10 as `1` in place of the `fchmod` failed at check-clang (`-Wunused-parameter`:  `mode` unused) before a test ran. M10 uses `((void)mode, 1)`:  `mode` stays used, and the copy keeps the mode of the `open` (0600). The first killer is `move EXDEV` (its EXDEV leg runs `fs_copy_file`;  e/f.txt has the mode 0600, not 0640), not `copy mode`:  make reaches the row `move EXDEV` first. `copy mode` also fails (3 rows fail). |
| D3 | The text of the M12 site occurs 2 times (`make_dir` line 738 and `trash_dir`). M12 edits the first only. |
| D4 | The terminal goldens hold the macOS form:  on Linux, trash-home uses the XDG trash under HOME/.local/share and its listDir of HOME/.Trash differs (kickoff section 5:  the goldens hold the macOS texts). The review fix removes the inherited XDG_DATA_HOME from the guest environment so that this trash also stays under the fixture HOME. |
| D5 | The chunk B D-rows D1, D3, D4 and D5 still have no row (the path regression count stays). The PLAN S6b note names them as not tested. |

#### Chunk C staged review (2026-10-21)

`fixture_env` now removes the inherited XDG_DATA_HOME: on Linux that
variable takes precedence over HOME and could send a fixture file to the
parent's trash directory. It also checks the length returned by
`snprintf`. A working directory that fits the 512-byte cwd buffer can
still leave too little room for the HOME suffix; the guest must not run
with that truncated path.

test/terminal-env.c checks the copied environment, preservation of the
parent environment, other fixtures and an overlong HOME. Before the
fix, the external-root and long-path checks fail. Makefile runs these
four checks as part of `make check` and includes the source in check-clang.

## S6 close:  the filesystem operations 2 to 10 (2026-10-21)

S6 is DONE. S6a added the filesystem read operations 2, 4, 6, 8 and 10
(`listDir`, `find`, `readFile`, `search`, `diskUsage`). S6b added the
write operations 3, 5, 7 and 9 (`makeDir`, `move`, `copy`, `trash`).
The rulings Q1 to Q16 are (a) (docs/PLAN.md, `### USER ruling (S6)`).
The base is 8534b3b, the USER commit of S6b chunk C with the USER review
fix (D1). The close changes the docs only:  docs/PLAN.md (the row S6b)
and docs/STATUS.md (the trash probe in S6b chunk C, and this section).

| Suite | S5 end | S6 end |
|---|---|---|
| examples | 32 | 33 checked, 0 failed |
| parse refusals | 14 | 14 |
| check refusals | 81 | 86 |
| evals | 163 | 179 |
| json | 32 | 33 |
| json refusals | 4 | 4 |
| gate | 0 failures | 0 failures |
| regressions | 6 | 6 checked, 0 failed |
| printer regressions | 2 | 2 |
| path regressions | 206 | 262 checked, 0 failed |
| terminal environment | (no suite) | 4 checked, 0 failed |
| terminal | 12 | 21 checked, 0 failed |

`make -B check-clang` (`cc -fsyntax-only`) gives rc 0. `git diff
--check` and the dash sweep are empty.

| Id | Chunk | Mutant | First killer | Result |
|---|---|---|---|---|
| M1 | S6a A | ioBind leg `inl`:  the searchOp arm gives `q p` | gate.sh `FAIL eval io searchTrace` | KILLED |
| M2 | S6a A | `find`:  `findOp ... g r` | gate.sh `FAIL eval io findTrace` | KILLED |
| M3 | S6a C | `fs_walk`:  the skip of `.` and `..` only when `deep` is not 0 | path regression `listDir sorted` | KILLED |
| M4 | S6a C | `fs_entry`:  `stat` in place of `lstat` | path regression `listDir sorted` | KILLED |
| M5 | S6a C | `read_file`:  the Text length from `strnlen` | path regression `readFile NUL` | KILLED |
| M6 | S6a C | `search_text`:  no 8 KiB NUL test | path regression `search binary` | KILLED |
| M7 | S6a C | `disk_usage`:  `total` from `f_bsize` in place of `f_frsize` | path regression `diskUsage values` | KILLED |
| M8 | S6b A | `copy`:  `copyOp ... t f` (from and to swapped) | gate.sh `FAIL eval io copyTrace` | KILLED |
| M9 | S6b C | `move_path`:  no `fs_there (TO)` test | path regression `move destination there` | KILLED |
| M10 | S6b C | `fs_copy_file`:  `((void)mode, 1)` in place of the `fchmod` | path regression `move EXDEV` | KILLED |
| M11 | S6b C | `trash_macos`:  `unlink (path)` in place of `rename (path, dest)` | path regression `trash macOS form` | KILLED |
| M12 | S6b C | `make_dir`:  one `mkdir` of the full path | path regression `makeDir parents` | KILLED |
| M13 | S6b C | `layer_of`:  moveOp gives `LAYER_COPY` | path regression `move file` | KILLED |

13 of 13 S6 mutants KILLED. The mutant rows of each chunk give the
lines and the number of rows that fail.

Open items:

- S6b chunk B D1:  no path regression tests an empty `makeDir` path.
- S6b chunk B D3:  no path regression tests a failed copy leg of an
  `EXDEV` move (the source and the partial destination stay).
- S6b chunk B D4:  no path regression tests an `EXDEV` move into its own
  tree. The copy has the row `copy into own tree`.
- S6b chunk B D5:  no row tests the fixed text of the kind (`copy
  cannot copy this kind`) or of no HOME (`there is no trash directory`).
- S6b chunk C D4:  the terminal goldens hold the macOS form. On Linux,
  trash-home uses the XDG trash under HOME/.local/share, and its listDir
  of HOME/.Trash differs from the golden. There are no Linux goldens.

| Id | Deviation |
|---|---|
| D1 | HEAD is 8534b3b, the USER commit of S6b chunk C with a review fix (tree 394220c4, not the staged tree 54cb58b7). Makefile:  the target build/terminal-env, which `make check` runs before build/terminal, and test/terminal-env.c in check-clang. test/terminal.c:  `fixture_env` removes XDG_DATA_HOME and refuses an overlong HOME (S6b chunk C, `#### Chunk C staged review`). test/terminal-env.c:  4 checks, the new suite `terminal environment`. The USER ruled (2026-10-21) to use HEAD as the base. |
| D2 | The prompt puts M1 to M7 in S6a chunk C. M1 and M2 ran in S6a chunk A (gate.sh killers at the trace defs). M3 to M7 ran in S6a chunk C. |
| D3 | docs/PLAN.md has no S6 milestone line. The rows S6a and S6b give the S6 status. The row S6b now records the trash probe, the suite `terminal environment` and the S6 close. |

## S7:  the ollama operation (2026-10-21)

### Chunk A:  layer and check

Chunk A adds the layer of the operation 1 `ollama` (S7 Q1, Q2 and Q3
(a)). domain/domain.lang adds the constructor 15 `ollamaOp` to IOF after
`trashOp` (3 fields:  `ollamaPath : Text`, `ollamaBody : Json` and
`ollamaNext : Sum Text Json -> X`). Thus each S6 fold arm keeps its
place. It adds the def `ollama : Text -> Json -> IO (Sum Text Json)` in
the form of `trash` after `trash`, and the arm in each leg of `ioBind`
and in `ioLoop`. Each fold over IOF now takes 15 functions, then the
target. examples/io.lang adds the arm in `layers` and `result`. The
`layers` arm gives `ollama`, the path and the `jsonRender` text of the
body. Each arm gives the fixed response `inr []`. The trace def
`ollamaTrace` binds `ollama "/api/generate"` of a body with 2 fields
into a `write` of `showJson` of the response. driver.c does not change
(chunk B):  `io_names` gets each constructor by its name, and an ollama
layer gives LAYER_OTHER until chunk B.

Precondition:  HEAD ba407a4 (the USER S6 close commit), tree ea4bdb43,
`git status --short` empty. `make -B check` GREEN at the S6 close
counts, `make -B check-clang` rc 0. The USER ruling:  Q1 to Q18 (a).

| Suite | Count |
|---|---|
| examples | 33 checked, 0 failed |
| parse refusals | 14 |
| check refusals | 88 |
| evals | 180 |
| json | 33 |
| json refusals | 4 |
| gate | 0 failures |
| regressions | 6 |
| printer regressions | 2 |
| path regressions | 262 checked, 0 failed |
| terminal environment | 4 checked, 0 failed |
| terminal | 21 checked, 0 failed |

`cc -fsyntax-only` (check-clang) passes. The dash sweep is empty.
`git diff --check` is clean.

The name probe tested the 6 names (`ollama`, `ollamaOp`, `ollamaPath`,
`ollamaBody`, `ollamaNext`, `ollamaTrace`) against src/,
test/regressions.js, test/terminal.c and the other examples:  no hit.

Edit sites:

- domain/domain.lang:  the IOF line, its comment, the IO monad comment,
  `def ollama` after `trash`, `ioBind` (2 legs) and `ioLoop`.
- examples/io.lang:  the comment, `layers`, `result` and `ollamaTrace`
  (with its comment). test/eval/expect.txt:  1 row. test/json/io.json:
  the trace text (the 23 instances of the S6 text do not change;  the
  24th is `ollamaTrace`).
- test/check:  io-ollama-nat-path.lang (`expected a term of type Text,
  found a term of type Nat`) and io-ollama-bind.lang (`the binder type
  Sum Text Text differs from the expected type Sum Text Json`), each
  TYPE_MISMATCH.
- test/path-regressions.c:  the arm `(fun p j k => 0)` in each of the 6
  guest folds over `out`, the budget of `ioLoop of 1000 rounds` and its
  2 comments (`370 at S7`).
- docs/PLAN.md:  the `### USER ruling (S7)` block after the S6 block
  (Q16 (a):  section 8 does not change).

Budget (one runner script under $TMPDIR;  the copy of the repo reads
the budget from PR_LOOP_BUDGET):

| Row | S6b minimum | S7 A minimum | Budget |
|---|---|---|---|
| `ioLoop of 1000 rounds` | 362 | 370 (369 gives status 1) | 380 |

| Id | Deviation |
|---|---|
| D1 | U1 is not set. The USER message gives U1 as the form `<set / not set>`. The sandbox `bind` of 127.0.0.1 at port 0 gives EPERM again (the kickoff probe binary, 2026-10-21). Chunk A does not need U1. Chunk B needs it. |
| D2 | The kickoff names no path and no body for the trace. `ollamaTrace` uses `/api/generate` and `{"model":"m","prompt":"hi"}`. The trace text holds the body as a JSON text in a JSON string. |
| D3 | The kickoff M2 names the refusal `ollama-nat-path`. The files are io-ollama-nat-path.lang and io-ollama-bind.lang (the S6 `io-` prefix). |
| D4 | `layer 3 operations at one segment` (`FS_SEGMENT_BUDGET` 305) passes with the new arm. This chunk did not probe its new minimum. |

| Id | Mutant | Killer |
|---|---|---|
| M1 | `ioBind` leg `inl`:  the ollamaOp arm gives `k (inl "")` in place of `k r` | gate.sh `FAIL eval io ollamaTrace` (KILLED;  then `FAIL json examples/io.lang`) |
| M2 | `ollamaPath : Nat` and `def ollama : Nat -> ...` (the binders follow, thus the domain loads) | gate.sh `FAIL examples/io.lang: langc: TYPE_MISMATCH: layers: the binder type Text differs from the expected type Nat` (KILLED;  then `FAIL io-ollama-nat-path.lang: want exit 1 and TYPE_MISMATCH, got exit 0` and the io evals) |

M1 and M2 ran after `git add`, each with `make -B check`, and were
restored with `git checkout -- domain/domain.lang` (`git diff --quiet`
gives 0). `make -B check` after the restore is GREEN. M3 to M10 run in
S7 chunk C (driver mutants).

### Chunk B:  driver and path regressions (PARTIAL, part 1 of 2)

Part 1 adds the driver of `ollama` (S7 Q2 to Q11, Q15 and Q18 (a)).
The mock server, the path regressions, the Makefile edit and the
SIGPIPE edits of the 2 runners are part 2. U1 is set:  the sandbox
`bind` probe gives rc 0 (2026-10-21).

Precondition:  HEAD e84f97a (the USER S7 A commit), tree 0d2ad698,
`git status --short` empty. `make -B check` GREEN at the chunk A counts,
`make -B check-clang` rc 0.

Edit sites:

- src/runtime/driver.h:  the IoEnv fields `ollama_host` and
  `ollama_timeout_ms`, OllamaEndpoint and `ollama_endpoint`.
- src/runtime/driver.c:  the includes (netdb.h, netinet/in.h,
  sys/socket.h, sys/time.h, front/jsontext.h), IoNames `ollama`,
  io_names (`ollamaOp`, 3 fields), LAYER_OLLAMA, layer_of, the io_step
  case and the functions `ollama_endpoint`, `ollama_path`, `ollama_url`,
  `ollama_connect`, `ollama_request`, `ollama_read` (the 64 MiB limit),
  `ollama_chunks`, `ollama_decode`, `ollama_status_fail`, `ollama_lines`
  and `ollama_call`.
- src/runtime/rt_main.c:  OLLAMA_HOST into IoEnv (an empty value is
  NULL) and the 2 initializers. rt_main.c already ignores SIGPIPE.
- test/path-regressions.c:  `pr_ollama_host`, `pr_ollama_timeout_ms` and
  the IoEnv initializer.

Gate:  `make -B check` GREEN at the chunk A counts (path regressions
262, terminal 21);  `make -B check-clang` rc 0. The dash sweep is
empty. `git diff --check` is clean. No new row in part 1.

| Id | Deviation |
|---|---|
| D1 | Chunk B is in 2 parts:  the session got to its hand-off limit after the driver. Part 1 has no new row. |
| D2 | The error texts that the kickoff does not name:  `ollama: PATH: the path does not start with /`, `ollama: PATH: the path holds a space or a control byte`, `ollama: the path holds a NUL byte`, `ollama: URL: the headers do not end with CRLF CRLF`, `ollama: URL: the status line is not valid`, `ollama: URL: a header line is not valid`, `ollama: URL: the Content-Length is not valid`, `ollama: URL: a chunk size is not valid`, `ollama: URL: a chunk does not end with CRLF`, `ollama: URL: the body does not render to a JSON text`, `ollama: URL: the write timed out`. The OLLAMA_HOST texts:  `https is not supported`, `a path is not supported`, `the host has no ]`, `the host is empty`, `the host is longer than 255 bytes`, `the text after the host is not a port`, `the port is not 1 to 65535`. |
| D3 | An OLLAMA_HOST value `HOST/` (a slash with no path) is a path:  `a path is not supported`. |

#### Staged review corrections

The review added ten C path regressions and a loopback mock in
`test/mock-ollama.c` (one forked child per response, port 0). Six rows
failed against the original staged driver. The corrections require the
blank line after the zero chunk and any trailers, accept trailing space
and tab in Content-Length, and preserve the full JSON error string,
including NUL bytes. A raw error body still stops at 512 bytes.

The path runner now ignores SIGPIPE and its Makefile dependency includes
the mock source. These review additions cover the three findings; the
remaining chunk B part 2 work above is still pending.

Validation in one isolated copy: `make check` and `make check-clang` rc 0;
path regressions 272, terminal environment 4, terminal 21, all with zero
failures. No existing assertion or check was removed or relaxed.
