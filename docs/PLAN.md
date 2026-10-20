# agent-lang plan

## 1. Rulings

The USER gave these rulings in the request (`design/DESIGN.md`):

- The type formers are products, coproducts, monads (map and bind),
  algebras and coalgebras (fold and unfold), and the quotients, paths,
  loops, QITs, HITs, QIITs and HIITs of cubical type theory (CTT).
- Proof irrelevance is prohibited (section 3).
- The base types are only these five: a text and bytes type that is UTF-8
  compatible, 64-bit integers, natural numbers, a JSON type and an
  append-only log type (section 5).
- The base operations are only these thirteen: call an Ollama endpoint and
  receive the responses, list a directory, make a directory, find an item,
  move, read a file, copy, search for text, move to the trash, disk usage,
  read a line from the terminal, write to the terminal, and get the process
  arguments (section 6).
- Host: TinyCC. Output: a console program (section 7).

These are the builder's calls. The USER did not rule them. Section 9 lists
them as open items:

- Name: agent-lang (O1). The compiler binary is `build/langc`.
- Host kit: `hosts/tcc-json` of lang-template, without change. The cubical
  kernel goes into this repository, not into lang-template (O2).
- Tests: unit and integration tests are in the guest language. A host
  runner in C that loads and runs guest test files is permitted. Do not add
  a node, Python or shell harness. The kit JS regressions stay.

### USER ruling (S1)

The USER gave this ruling on 2026-10-18, before S1:

RULINGS: O1=a O2=a O3=a O4=a O5=a O6=a O7=a O8=a O9=a O10=a O11=a O12=a O13=a O14=a O15=a O16=a O17=a O18=a O19=a O20=a

Each letter is the proposal (a) of section 9. Thus the slice order of
section 8 does not change.

Amendment (USER, 2026-10-18, after the S1 kickoff): also admit the `Fin`
type former. This replaces O5 (a). The coin-lang `Fin n` builtin and its
operations stay (sections 2 and 4). The other coin types go.

### USER ruling (S2)

The USER gave this ruling on 2026-10-19, after the S2 kickoff (the kickoff
record, outside the repo, section 4):

RULINGS: Q1=a Q2=a Q3=a Q4=a Q5=a Q6=a Q7=a Q8=a Q9=a

Each letter is the proposal (a) of the kickoff.  Q1: the orbit-lang grammar
verbatim (the index telescope after the parameters, the constructor result
after the fields and the interval fields and before the boundary, `and` for
a block, headers first, then point constructors, then path constructors).
Q2: Nat-indexed examples, `Vec` and the `Charter`/`Ratified` block in the
domain, no Size, no Bag, no squash constructor.  Q3: `elim` beside `fold`,
a new keyword and the Op OP_ELIM, `fold` unchanged.  Q4: no Prop rows;
section 3 is unchanged.  Q5: `Quot` stays the only quotient former.  Q6: the
new codes HIT_INDEX and BLOCK_ORDER;  the rest TYPE_MISMATCH, HIT_BOUNDARY
and PATH_END.  Q7: block sorts stay outside `elim` (TYPE_MISMATCH).  Q8:
transp at an indexed or block sort stays neutral for every line.  Q9:
families stay in domain/domain.lang (rule R1).  The slice order of section 8
does not change.

### USER ruling (S3)

The USER gave this ruling on 2026-10-19, after the S3 kickoff (the kickoff
record, outside the repo, section 2):

RULINGS: Q1=a Q2=a Q3=a Q4=a Q5=a Q6=a Q7=a Q8=a Q9=a

Each letter is the proposal (a) of the kickoff.  Q1: `Bool := Fin 2` as a
domain def with `true`, `false` and `boolIf`;  `Flag`, `flagYes`, `flagNo`
and `flagIf` go;  `natEq`, `natLe`, `textEq`, `textUtf8`, `int64Eq` and
`int64Le` give `Bool`.  Q2: the sample domain (`Color`, `Item`, `Stack`,
`Lot` and their defs) goes;  its users are rewritten over `Fin`, `Vec`,
`Json` and `Log`.  Q3: a Nat below 2^64 stays a machine word;  a larger
Nat is a limb value;  every Nat operation promotes and demotes.  Q4: a
Text is a JSON string when it is valid UTF-8, else `{"bytes":"HEX"}`.
Q5: a numeral checks at `Int64` by the Fin literal rule;  `int64Neg` gives
the negatives;  no `decEqInt64`.  Q6: a Nat in the JSON document is a JSON
number of any length.  Q7: `Json` is a domain family with the six
constructors of section 5, `jnum` over `Sum Int64 Text`;  `jsonParse` and
`jsonRender` are builtins bound to that family.  Q8: `Log` is a domain
family with `logEmpty` and `logAppend`;  `logFold` is `fold`;  `logLength`
is a def.  Q9: the Nat overflow trap goes;  the fuel trap stays.  The
slice order of section 8 does not change.

### USER ruling (S4)

The USER gave this ruling on 2026-10-20, after the S4 kickoff (the kickoff
record, outside the repo, section 2):

RULINGS: Q1=a Q2=a Q3=a Q4=a Q5=a Q6=a Q7=a Q8=a Q9=a Q10=a

Each letter is the proposal (a) of the kickoff.  Q1: a layer family with
one more parameter `X : Type 0`, then `cofamily C ps := F ps` in the domain
(rule R1).  Q2: a layer field does not use X, is X, or is a Pi chain with
the codomain X;  the layer is uniform, has point constructors only and is
a block of one.  Q3: coiteration only, `unfold g s : C ps` with
`g : S -> F ps S`;  the type is the guard;  R3 stays.  Q4: `unfold g s` is
a value;  `out` applies g once and spends 1 fuel;  no cache.  Q5: two
coinductive values convert when the steps and the seeds convert;  no eta;
no path from a bisimulation (open item O21).  Q6: transp along a constant
line is the identity, else neutral;  hcomp is formal.  Q7: a cofamily is
not an instance type;  a cofamily value inside an instance is JSON_VALUE.
Q8: the kit `List` unfold stays;  the hint selects the rule.  Q9: the new
codes CODATA_FORM, CODATA_POSITIVE and CODATA_FOLD.  Q10: Stream, Colist
and Mealy in the domain.  The slice order of section 8 does not change.

## 2. The request on the kit basis

| Concept | Request text | Encoding (proposal) | Slice |
|---|---|---|---|
| Products | products | `Sigma (x : A) B`, pairs and projections. The kit has them; coin-lang adds paths in Sigma. | S1 |
| Coproducts | coproducts | `Sum A B`, `inl`, `inr` and the case fold. `Unit` and `Empty` are the empty product and the empty coproduct. `Bool := Sum Unit Unit` is a library def. | S1 |
| Monads | monads (map/bind) | The kit `pure`, `map` and `bind` over `Option`, `Sum E` and `List` (kit F5), and over `IO` (section 6). The monad laws are paths, not conversions. | S1, S5 |
| Algebras | algebras (fold) | `fold` over `Nat`, `List` and each domain family (kit F6), with the path cases of coin-lang S4. | S1 |
| Coalgebras | coalgebras (unfold) | The kit `unfold` goes into `List` with a `Nat` step limit (kit F7). That is not a final coalgebra. Proposal: coinductive families with a guarded `unfold` and a lazy `out` (O4). | S4 |
| Paths and loops | path/loops | The interval, `PathP`, `transp`, `hcomp`, Glue and `ua` of coin-lang 61f1c8e (coin-lang S1 to S3). | S1 |
| Quotients, QITs, HITs | quotients, QITs, HITs | Path constructors, square constructors, `S1`, `Trunc1`, `Trunc0` and `Quot A R` of coin-lang S4a to S4c. | S1 |
| QIITs, HIITs | QIITs, HIITs | Indexed families and mutual blocks (`and`) of orbit-lang S6 (6fa6a15, 8dd82a3, 8b11846), and the dependent eliminator of orbit-lang 676c7a8. | S2 |
| Higher kinded types | higher kinded types | A type operator is a term of `U -> U` (a Pi type into the universe). A family takes type parameters. There is no new former. | S1 |
| Finite types | (USER amendment, section 1) | `Fin n` of coin-lang 61f1c8e: the numerals, `finCase`, `finAll`, the Fin ring, `decEqFin`, `isSetFin`, and the Fin transport tables in JSON (O5). | S1 |

## 3. The no-irrelevance rule

A proof is data. A path is data. The kernel, the JSON writer and the
console program must keep them. These are prohibited:

- a universe with definitional proof irrelevance (`Prop` or `SProp`);
- UIP or axiom K, as an axiom or as a conversion rule;
- a conversion rule that makes two paths equal because they have the same
  type or the same end points;
- a groupoid law as a conversion (for example `p . sym p` to `refl`). A
  groupoid law is a 2-path;
- a JSON writer that erases a path or a proof;
- a console program that erases a path or a proof. Section 7 keeps the
  core terms, so a transport along `ua e` computes at run time.

These are permitted, because they do not identify proofs by their type:

- the CTT computation rules: the end point rules of `PathP`, the rules of
  `transp`, `hcomp`, Glue and the HIT eliminators, and uaβ;
- `Trunc1` and `Trunc0` as HITs. Their truncation constructors are path
  constructors: they add paths and they identify no terms by conversion;
- `isProp A` and `isSet A` as ordinary types (Pi types of paths);
- path eta, and the normal form of the interval (`~~r = r` and the De
  Morgan laws), as in coin-lang ruling R11.

## 4. The kernel route

The kernel is a port. It is not a new design.

- S1 ports the coin-lang kernel at 61f1c8e (coin-lang S8, closed):
  `src/front/*`, `src/json.c` and the library defs of
  `domain/domain.lang` (`J`, `transport`, `trans`, `Equiv`, `isoToEquiv`,
  `ua`, `uaArrow`, `S1`, `Trunc1`, `Trunc0`, `Quot`, `DecEq`, Hedberg).
  coin-lang and this repository come from the same kit (tcc-json), so the
  port is a copy of the kernel files and a removal of the coin domain
  (`FinSet`, `Rand`, `couple`, `Bag`, the Schnorr and graph isomorphism
  examples).
- S2 ports the indexed families, the mutual blocks and the dependent
  eliminator of orbit-lang S6 and S7. orbit-lang comes from the
  tcc-evm-contract kit, so S2 ports the diffs of its front files, not the
  files.
- The coin-lang `Fin n` builtin, `finAll`, `finCase`, the Fin ring and
  the `Fin` transport tables (coin-lang JSON rule 1) stay. The USER
  admitted `Fin` (section 1, amendment; O5).

## 5. The base types

The request gives the base types and no pure operations on them. This
plan reads it so: each base type has its introduction and elimination
forms, and the thirteen operations of section 6 are the only effects
(O6). The proposal keeps the pure operations to the minimum that the
section 6 operations need.

| Type | Request text | Values | Pure operations (proposal) | Open |
|---|---|---|---|---|
| `Text` | a text/bytes type that is UTF-8 compatible | A byte string. A literal in the source is UTF-8. `readFile` can give bytes that are not UTF-8. | `textAppend`, `textLength` (bytes), `textEq : Text -> Text -> Bool`, `textSlice`, `textUtf8 : Text -> Bool` | O7 |
| `Int64` | 64-bit integers | Two's complement, 64 bits. | `+`, `-`, `*` modulo 2^64; `div` and `rem` give `Option Int64` (`none` at a zero divisor); compare; `int64ToNat : Int64 -> Option Nat`, `natToInt64 : Nat -> Option Int64` | O8 |
| `Nat` | Natural numbers | The kit `zero`, `succ` and `fold`. The run time value has no bound (a big number in C). | literals, `natAdd`, `natMul`, `natSub` (truncated), compare, `natShow : Nat -> Text` | O9 |
| `Json` | a JSON type | A kernel family: `jnull`, `jbool`, `jnum`, `jstr`, `jarr (List Json)`, `jobj (List (Sigma Text Json))`. `fold` and paths work as for any family. | `jsonParse : Text -> Sum Text Json`, `jsonRender : Json -> Text` | O10 |
| `Log` | an append-only log type | A snoc list of `Json` entries. The only constructors are `logEmpty` and `logAppend : Log -> Json -> Log`. There is no remove, no update and no prepend. | `logFold`, `logLength` | O11 |

The name `Path` is the CTT path type. A filesystem path is a `Text`.

## 6. The base operations

### 6.1 Effects

The checker and `langc eval` never run an operation. A program describes
its effects as a value of `IO A`, and the console program runs that value.

Proposal (O3 (a)): `IO A` is the final coalgebra of the functor
`X |-> Sum A (Sigma (o : Op) (Resp o -> X))`, an interaction tree over the
signature of the thirteen operations. `pure` and `bind` make it a monad,
and `unfold` makes a loop (for example a read, ask, write loop at the
terminal). This uses only the formers of the request. The console runtime
walks the tree: at an `Op` node it runs the operation and gives the
response to the continuation. S4 (coinductive families) comes before S5.

Alternative (O3 (b)): `IO` is a primitive former with `pure`, `bind` and a
loop combinator. This needs no S4, but it adds a former that the request
does not name.

A program has `def main : IO Unit`.

### 6.2 The operations

Each failure is a value: `Sum Text R` gives `inl` with an error text.

| # | Name and type (proposal) | Request text | Run time (C, POSIX) | Open |
|---|---|---|---|---|
| 1 | `ollama : Text -> Json -> IO (Sum Text Json)` | call an Ollama endpoint and receive responses | HTTP/1.1 POST of the body to the endpoint path (for example `/api/chat`) at `OLLAMA_HOST`, default `127.0.0.1:11434`, on a TCP socket. No TLS. The result is a JSON array of each response object: one object when the body has `"stream": false`, one for each NDJSON line when it streams. | O12 |
| 2 | `listDir : Text -> IO (Sum Text Json)` | list the contents of a directory | `opendir` and `readdir`. An array of `{"name", "kind"}`, sorted by name, without `.` and `..`. | |
| 3 | `makeDir : Text -> IO (Sum Text Unit)` | make a directory | `mkdir` with the parents. No error when the directory is there. | O13 |
| 4 | `find : Text -> Text -> IO (Sum Text Json)` | find an item | Walk the root. Match each base name to a glob (`fnmatch`). An array of paths. Do not follow symbolic links. | |
| 5 | `move : Text -> Text -> IO (Sum Text Unit)` | move something from one place to another | `rename`. At `EXDEV`, copy and then remove the source. Refuse when the destination is there. | O13 |
| 6 | `readFile : Text -> IO (Sum Text Text)` | read a file | The bytes of the file. | |
| 7 | `copy : Text -> Text -> IO (Sum Text Unit)` | copy something from one place to another | A file, or a directory with its contents. Keep the mode bits. Refuse when the destination is there. | O13 |
| 8 | `search : Text -> Text -> IO (Sum Text Json)` | search for text | A fixed string (not a regular expression) under a root file or directory. An array of `{"path", "line", "text"}`. Skip a file with a NUL byte in its first 8 KiB. | O14 |
| 9 | `trash : Text -> IO (Sum Text Unit)` | move something to the trash | macOS: `rename` into `~/.Trash`, with a suffix at a name collision. Linux: the XDG trash (`files/` and `info/*.trashinfo`). Never `unlink`. | O15 |
| 10 | `diskUsage : Text -> IO (Sum Text Json)` | display filesystem disk usage | Proposal (a): `statvfs` of the filesystem that holds the path: `{"total", "free", "available"}` in bytes (as `df`). | O16 |
| 11 | `readLine : IO (Option Text)` | read a line from the terminal | One line of standard input without the newline. `none` at the end of the input. | |
| 12 | `write : Text -> IO Unit` | write something to the terminal | The bytes to standard output, then a flush. No newline is added. | O17 |
| 13 | `args : IO Json` | retrieve process arguments | An array of the arguments after the program name. | O18 |

The request has no file write, no delete, no process spawn and no network
other than Ollama. A `Log` goes out only through `write` (O11).

## 7. The console program

`langc build PROG -o OUT` checks `PROG`, finds `main : IO Unit` and writes
the native executable `OUT`.

Proposal (O19 (a)): `langc` links libtcc (`~/.local/lib/libtcc.a`, TinyCC
0.9.28rc). It compiles one C unit in memory: the runtime (the evaluator,
the `IO` driver and the thirteen operations) and the checked core program
as data. The runtime uses the same evaluator as `langc eval`, so the
`transp`, `hcomp`, Glue and HIT rules compute at run time and no path is
erased (section 3). Alternative (O19 (b)): compile the core to C closures.
That is faster, and it is a later slice when (a) is too slow.

Exit status of `OUT`: 0 when `main` ends; 1 at a run time refusal (for
example the arena limit). An operation failure is a value, not an exit.

`langc check` and `langc eval` stay. The kit `build` verb writes the JSON
document of the instances; it becomes `langc json` and stays as a test aid
(O20).

## 8. Slices and gates

| Slice | Contents |
|---|---|
| S0 | This seed: the kit, the design, this plan. |
| S1 | The coin-lang kernel port (section 4). Remove the coin domain. DONE 2026-10-19: rulings K1 to K7 = (a), Fin admitted; `docs/STATUS.md`, section S1. |
| S2 | The orbit-lang indexed families, mutual blocks and dependent eliminator: QIITs and HIITs. DONE 2026-10-19: rulings Q1 to Q9 = (a); `docs/STATUS.md`, section S2. |
| S3 | The base span: remove the kit sample types that the request does not give, add `Text`, `Int64`, `Nat` (no bound), `Json` and `Log` with the section 5 operations. DONE 2026-10-20: rulings Q1 to Q9 = (a); `docs/STATUS.md`, section S3. |
| S4 | Coinductive families: guarded `unfold`, lazy `out` (O4). |
| S5 | `IO`, the operation signature, `langc build` with libtcc, and the terminal operations 11 to 13. |
| S6 | The filesystem operations 2 to 10, tested in a new directory under `build/`. |
| S7 | `ollama`, tested against a mock server in C that TinyCC builds, and a live probe when a local Ollama runs. |
| S8 | Examples (a chat loop that keeps a `Log` transcript, a file organizer), mutants, docs. |

Each slice ends with `make check` GREEN, guest test files for each new
rule and refusal, and a mutant table (each mutant killed or recorded).
Each slice after S0 is a USER commit.

## 9. Open items

Each item gives the proposal first.

- O1 Name: agent-lang. Rename before S1 if the USER prefers another name.
- O2 Kernel home: this repository. lang-template stays without a cubical
  kernel.
- O3 Effects: (a) `IO` as the final coalgebra of the operation signature;
  (b) a primitive `IO` former.
- O4 Coalgebras: (a) coinductive families with a guarded `unfold`; (b) keep
  the kit `unfold` with a step limit only (then O3 must be (b)).
- O5 `Fin n`: (a) remove it with the coin domain; (b) keep it as a library
  family over `Nat`. RULED (USER, 2026-10-18): neither. Keep the coin-lang
  `Fin n` builtin and its operations (section 1, amendment).
- O6 Pure operations: (a) each base type has its introduction and
  elimination forms and the section 5 minimum; (b) no pure operation that
  the request does not name.
- O7 `Text`: (a) one byte type, UTF-8 literals, a validity test; (b) a
  `Text` that is always valid UTF-8 and a separate bytes type (the request
  names one type).
- O8 `Int64`: (a) arithmetic modulo 2^64, `Option` at a zero divisor;
  (b) checked arithmetic that gives `Option` at each overflow.
- O9 `Nat`: (a) no bound at run time; (b) 64-bit, with a refusal at
  overflow.
- O10 JSON numbers: (a) a number is an `Int64` when it is an integer that
  fits, else its decimal text; (b) every number is its decimal text.
- O11 `Log`: (a) entries are `Json`; (b) `Log A` for each type `A` (a type
  former, not a base type).
- O12 `ollama`: (a) any endpoint path, the array of each response object;
  (b) `/api/chat` only.
- O13 Overwrite: (a) `move` and `copy` refuse an existing destination, and
  `makeDir` makes the parents; (b) they replace it.
- O14 `search`: (a) a fixed string; (b) a regular expression (POSIX
  `regcomp`).
- O15 `trash` on Linux: (a) the XDG trash; (b) macOS only.
- O16 Disk usage: (a) the filesystem that holds the path (`df`);
  (b) the size of the path and its contents (`du`); (c) both, in one
  object.
- O17 `write`: (a) standard output only; (b) also standard error.
- O18 `args`: (a) the arguments after the program name; (b) all.
- O19 Output: (a) libtcc in `langc`, the runtime evaluator in the
  executable; (b) the core compiled to C closures.
- O20 The kit `build` verb: (a) rename it to `langc json`; (b) remove it.
