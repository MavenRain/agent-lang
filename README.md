# agent-lang

agent-lang is a language for console programs that use the filesystem,
the terminal and a local Ollama model. The type formers are products,
coproducts, monads (map and bind), folds and unfolds, and the quotients,
paths, loops, QITs, HITs, QIITs and HIITs of cubical type theory (CTT).
The base types are text, 64-bit integers, natural numbers, JSON and an
append-only log. The kernel does not identify proofs: there is no proof
irrelevance. `langc` checks a program and writes a console program. The
language comes from lang-template with the tcc-json host (TinyCC). The
build plan is `docs/PLAN.md`. S1 (the coin-lang kernel port) is in: paths,
`transp`, `hcomp`, Glue, `ua`, the HITs `S1`, `Trunc1`, `Trunc0` and `Quot`,
`Fin`, and the builtins `Empty` and `absurd` (`docs/STATUS.md`, section S1).
S3 (the base span) is in (`docs/STATUS.md`, section S3). A `Text` holds
bytes. An `Int64` wraps. A `Nat` has no bound. `Json` is the domain tree
that `jsonParse` and `jsonRender` read and write. A `Log` is a snoc list
of `Json` (`logEmpty`, `logAppend`).

Coalgebras (S4, `docs/STATUS.md`, section S4): the domain gives the
cofamilies `Stream A`, `Colist A` and `Mealy I O`. `unfold g s` makes a
value from the step `g` and the seed `s`, and `out` gives one layer. A
stream gives a head and a tail, a colist can stop, and a Mealy machine
gives an output and the next machine for each input. Each `out` applies
`g` one time. There is no path from a bisimulation (Q5): two unfolds are
equal only when their steps and their seeds are equal.

Console programs (S5, `docs/STATUS.md`, section S5): a program with
`def main : IO Unit` is a console program. `langc build PROG -o OUT`
checks `PROG` and writes the executable `OUT` with libtcc
(`~/.local/lib/libtcc.a`). The terminal operations are
`readLine : IO (Option Text)` (a line without its newline, or `none` at
the end of the input), `write : Text -> IO Unit` (the bytes, then a
flush) and `args : IO Json` (an array of the arguments after the program
name). `pure`, `map` and `bind` work at `IO`, and `ioLoop` makes a loop.
`OUT` exits 0 when `main` ends. It exits 1 at a run time refusal (for
example EVAL_FUEL, OOM or IO_WRITE), with the diagnostic on stderr.
`langc build` refuses a program with no `main` (BUILD_MAIN) or with a
`main` that is not `IO Unit` (BUILD_MAIN_TYPE). A libtcc error is
BUILD_TCC. `langc json PROG [-o OUT]` writes the JSON document of the
instances. `examples/io.lang` has an echo loop, a greeting from the
arguments and a line counter.

Filesystem reads (S6a, `docs/STATUS.md`, section S6a): a console program
can use `listDir : Text -> IO (Sum Text Json)` (the names and the kinds
in a directory), `find : Text -> Text -> IO (Sum Text Json)` (the paths
under a root whose base name matches a glob),
`readFile : Text -> IO (Sum Text Text)` (all the bytes of a file),
`search : Text -> Text -> IO (Sum Text Json)` (each line that holds a
fixed text) and `diskUsage : Text -> IO (Sum Text Json)` (the bytes of
the filesystem, from `statvfs`). A walk does not follow a symbolic link.
A failure is a value: `inl` of an error text. OS errors use
`OP: PATH: TEXT`; input validation can omit the path.
`examples/fs.lang` reads sample responses. `formers/tcc-json.md`,
section `## S6 shapes`, gives the JSON forms.

Filesystem writes (S6b, `docs/STATUS.md`, section S6b): a console
program can use `makeDir : Text -> IO (Sum Text Unit)` (a directory and
its parents), `move : Text -> Text -> IO (Sum Text Unit)`,
`copy : Text -> Text -> IO (Sum Text Unit)` (a file, a link as a link,
or a directory tree with the mode bits) and
`trash : Text -> IO (Sum Text Unit)` (macOS `HOME/.Trash`, Linux the
XDG trash). Each gives `inr unit`, or `inl` of `OP: PATH: TEXT`. `move`
and `copy` refuse a destination that is there. No operation deletes a
path:  `trash` moves it.

- `SPEC.md`: the specification. Fill each section.
- `design/DESIGN.md`: the domain design. It is the only source of the core
  types and the core operations.
- `formers/FORMERS.md`: the type formers. `formers/tcc-json.md` gives their
  status on the tcc-json host.
- `probe/CAPABILITY.md`: the host probe.
- `docs/STATUS.md` and `docs/VALIDATION.md`: the status and the gate results.
- `docs/host/README.md`: how to build and test the host kit, and how to
  replace the sample domain in `domain/`.
