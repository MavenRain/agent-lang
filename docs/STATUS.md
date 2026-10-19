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
