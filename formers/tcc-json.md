# TinyCC JSON realization

Paths below are relative to this host kit, or to the root of a generated
language. The template's `formers/FORMERS.md` gives the host-neutral laws.
The front end in `src/front/` is a fork of the tcc-wasm front end (`README.md`,
section Origin). Each status covers `langc check`, `langc eval` and
`langc build`. `test/gate.sh` checks every example, every eval line in
`test/eval/expect.txt` and every JSON golden in `test/json/`. `make check`
also runs `test/regressions.js` for type safety, complete type metadata,
binder scope and domain declarations.

| ID | Former | Status and evidence |
|---|---|---|
| F1 | Product | DONE: `pair`, `first`, `second`; `examples/formers.lang`, `examples/functions.lang` |
| F2 | Coproduct | DONE: `inl`, `inr`; `examples/formers.lang`, `examples/laws.lang` |
| F3 | Option | DONE: `none`, `some`; `examples/formers.lang`, `examples/monad.lang` |
| F4 | List | DONE: `nil`, `cons`; `examples/formers.lang`, `examples/inventory.lang` |
| F5 | Monad pure/map/bind | DONE over Option, Sum E and List: `examples/monad.lang`, `examples/laws.lang` |
| F6 | Algebra fold | DONE over Nat, List and each family: `examples/algebra.lang`, `examples/entries.lang` |
| F7 | Algebra unfold | DONE into List with a Nat step limit: `examples/algebra.lang` |
| F8 | Filter | DONE over Option and List: `examples/monad.lang`, `domain/domain.lang` |
| F9 | Non-dependent Pi | DONE: `examples/functions.lang` |
| F10 | Dependent Pi | DONE: `examples/functions.lang`, `examples/universes.lang` |
| F11 | Sigma | DONE: `pack`, `witness`, `payload`; `examples/sigma.lang` |
| F12 | Eq refl/symm/trans | DONE: `examples/equality.lang`, `examples/laws.lang` |
| F13 | Eq transport/cong | DONE: `examples/equality.lang` |
| F14 | Universes | DONE: `Type 0` and `Type 1`, without cumulativity; `examples/universes.lang`, `test/check/cumulative.lang`, `test/check/type2.lang` |
| F15 | Generic indexed family | DONE in `domain/` only (rule R1): `Lot` in `domain/domain.lang`, `examples/inventory.lang` |

JSON output of each former: `README.md`, section The JSON document.
Functions, types, families and equality proofs are not instances. Thus
F9, F10, F14 and the F12 and F13 proofs have no JSON value of their own.
A Sigma payload of `Eq` type is `null`.

S1 rows (the coin-lang 61f1c8e kernel port, 2026-10-19). Each status
covers `langc check`, `langc eval` and `langc build`.

S1 replaces the kit `Eq` encoding above: paths, including Sigma payloads,
retain their structure. Formal `hcomp` objects include `phi`, `tube` and
`base`; `phi` and `tube` are source strings in the same interval context.

| ID | Feature | Status and evidence |
|---|---|---|
| C1 | Paths: `PathP`, `Path`, `@`, `refl` | DONE: `examples/paths.lang`, `examples/equality.lang` |
| C2 | `transp` | DONE: `examples/kan.lang` |
| C3 | `hcomp` | DONE: `examples/kan.lang`, `examples/circle.lang` |
| C4 | Glue, `glue`, `unglue` | DONE: `examples/glue.lang` |
| C5 | `ua`, `uaArrow` | DONE: `examples/univ.lang`, `examples/equiv.lang` |
| C6 | HITs `S1`, `Trunc1`, `Trunc0`, `Quot` | DONE: `examples/circle.lang`, `examples/trunc.lang`, `examples/quot.lang` |
| C7 | `Fin` and the Fin transport tables in JSON | DONE: `examples/fin.lang`, `examples/dec.lang`, `test/json/fin.json` |
| C8 | The builtins `Empty` and `absurd` | DONE: `examples/dec.lang`, `test/path-regressions.c` |

## S2 shapes: indexed constructors, block folds and elims

The three S2 goldens show the shapes that S2 adds. The emitter does not
change the 20 S1 goldens.

- An indexed constructor value is an object with `tag` and one member for
  each field, in field order. The index arguments of the type are not
  fields, so they do not appear in the value (`test/json/indexed.json`):
  `{"name":"vecTwo","type":"Vec (Fin 2) 2","value":{"tag":"vcons","consLen":1,"consHead":0,"consTail":{"tag":"vcons","consLen":0,"consHead":1,"consTail":{"tag":"vnil"}}}}`.
  A path constructor of an indexed family keeps the S1 shape
  `{"path":NAME,"args":[...]}`, for example `{"path":"vswap","args":[0,0,1,{"tag":"vnil"}]}`.
- A value of a block sort has the same object shape. A field of a sibling
  sort is printed at the sibling type: the `adopt` value of `Charter Nat`
  holds a `Ratified Nat (genesis Nat)` field, and the single constructor
  of `Ratified` prints without a `tag` (`test/json/mutual.json`):
  `{"name":"charterOne","type":"Charter Nat","value":{"tag":"adopt","adoptBase":{"tag":"genesis"},"adoptRule":{"ratifyBase":{"tag":"genesis"},"ratifyRule":7}}}`.
  A block fold that computes is a plain value: `{"name":"adoptedOne","type":"Nat","value":7}`.
- An elim that computes is a plain value (`{"name":"vecTwoCountE","type":"Nat","value":2}`)
  or a refl (`{"name":"quotEqAtE","type":"PathP (fun _ => Trunc0 (Fin 4)) (trunc0 (Fin 4) 2) (trunc0 (Fin 4) 2)","value":{"refl":{"trunc":2}}}`).
  A stuck elim on a formal `hcomp` is printed as a path with `from`, `to`
  and the `term` as source text (`test/json/elim.json`, `hcompE`):
  `"value":{"from":{"tag":"vcons",...},"to":{"tag":"vcons",...},"term":"fun j => elim (fun k => fun w => Vec (Fin 2) k) (vnil (Fin 2)) (fun _x0 => ... ) (hcomp (Vec (Fin 2) 2) (j \/ ~j) (fun i => [ ... ]) (...))"}`.

## S3 shapes: Text, Int64, big Nat and JSON

The three S3 goldens (`test/json/text.json`, `test/json/int64.json`,
`test/json/json.json`) show the shapes that S3 adds. The emitter does not
change an S2 shape.

- A `Text` that is valid UTF-8 is a JSON string. The emitter writes `"`,
  `\` and each byte below 0x20 as `\u00xx` (lowercase) and copies all the
  other bytes: `{"name":"greeting","type":"Text","value":"hello, world"}`.
- A `Text` that is not valid UTF-8 is an object with one member `bytes`:
  two lowercase hex digits for each byte (Q4):
  `{"name":"rawBytes","type":"Text","value":{"bytes":"c080"}}`.
- An `Int64` is a JSON number in decimal, also when it is negative:
  `{"name":"wrapped","type":"Int64","value":-9223372036854775808}`.
- A `Nat` at or above 2^64 is a bare JSON number of any length (Q6 (a)),
  not a string: `{"name":"bigSum","type":"Nat","value":18446744073709551616}`.
  A reader that keeps a number as a double loses the low digits.
- A `Json` value has the family shape. `jnum` holds a `Sum Int64 Text`:
  `{"inl":7}` for an integer that fits Int64, `{"inr":"1.5"}` for each
  other number text (`test/json/json.json`, `doc`):
  `{"tag":"jnum","jnumValue":{"inr":"1.5"}}`.
