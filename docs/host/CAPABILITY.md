# TinyCC JSON host capability

This file holds host facts only. Put the facts of your domain in the
`probe/CAPABILITY.md` of your language. Paths are relative to the host
kit, or to the root of a generated language.

## Toolchain

- TinyCC 0.9.28rc (AArch64 Darwin) builds `build/langc` with
  `-std=c99 -Wall -Werror` (`Makefile`).
- `cc -Wall -Wextra -Wswitch-enum -Werror -fsyntax-only` checks the same
  sources (`make check-clang`). Each switch on an enum names every case
  and has no default arm.
- `tcc -run gen/embed.c` turns `domain/domain.lang` into `build/domain.c`
  at build time. The domain is part of the executable.

## Values

- `Nat` is an unbounded natural number. Values below 2^64 use a machine
  word; larger values use limbs. Arithmetic promotes and demotes as needed.
  `natSub` is truncated at 0. Nat arithmetic does not produce an overflow trap.
- `Text` holds bytes. Append, equality, byte slicing, length and UTF-8
  validation operate on those bytes. Slice bounds are clamped to the length.
- `Int64` holds signed 64-bit integers. Add, subtract, multiply and negate
  wrap. Division and remainder return `none` for a zero divisor and for
  the minimum integer divided by -1. Conversions return `none` out of range.
- `jsonParse` and `jsonRender` use the domain's `Json`, `JsonList` and
  `JsonFields` families. The checker validates their constructors and field
  types before admitting a JSON primitive (`JSON_FAMILY` on a mismatch).
- Values carry no types. The JSON writer gets each type from the
  definition type and from the family declarations. A valid UTF-8 `Text`
  emits as a JSON string, other bytes as `{"bytes":"HEX"}`, and a large
  `Nat` as a bare decimal number. An instance containing a trap is refused
  with `EVAL_TRAP`.

## Limits

- The checker and the evaluator stop at a depth of 2000
  (`src/front/check.c:10`, `src/front/core.h:16`), and the parser at a
  nesting of 1000 (`src/front/parser.c:16`). These limits keep the C stack
  bounded. `test/gate.sh` checks a nesting of 1100 parentheses.
- The evaluator has a fuel of 20,000,000 steps for each instance
  (`src/front/core.h:17`). `langc build` starts the fuel again for each
  instance.
- All memory comes from one arena with a limit of 1 GiB (`src/main.c:10`).
  A run that reaches it stops with `OOM`.
- Source files are at most 1 MiB (`src/front/front.h:7`).

## Output

- `langc build` makes the full JSON document in the arena, then writes
  it. A refused build writes no partial document and no `-o` file.
- The writer walks list spines in a loop, so a long list does not use C
  stack. Other nesting counts against the JSON depth of 2000
  (`src/json.h:15`).
