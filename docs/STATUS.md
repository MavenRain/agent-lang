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
