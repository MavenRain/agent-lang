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

- `SPEC.md`: the specification. Fill each section.
- `design/DESIGN.md`: the domain design. It is the only source of the core
  types and the core operations.
- `formers/FORMERS.md`: the type formers. `formers/tcc-json.md` gives their
  status on the tcc-json host.
- `probe/CAPABILITY.md`: the host probe.
- `docs/STATUS.md` and `docs/VALIDATION.md`: the status and the gate results.
- `docs/host/README.md`: how to build and test the host kit, and how to
  replace the sample domain in `domain/`.
