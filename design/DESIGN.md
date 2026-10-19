# agent-lang domain design

> Paste the domain design below the line, verbatim. Do not edit it.

This file is the only source of the core types and the core operations of
agent-lang. `SPEC.md` sections 5 and 6 cite its sections. Do not add a core
type or a core operation that this file does not give.

When the design changes, paste the new version. Then record the change as an
open item in `SPEC.md` section 9.

---

I want to write a new programming language with the following features:

For type constructors and higher kinded types, admit products, coproducts, monads (map/bind), algebras/coalgebras (fold/unfold), and quotients/path/loops/QITs/HITs/QIITs/HIITs as in Cubical TT.  Proof irrelevance is prohibited.

For base types, admit only a text/bytes type that is UTF-8 compatible, 64-bit integers, Natural numbers, a JSON type, and an append-only log type.  For base operations, admit operations to call an Ollama endpoint and receive responses, list the contents of a directory in the filesystem, make a directory in the filesystem, find an item in the filesystem, move something in the filesystemm from one place to another, read a file from the filesystem, copy something from one place to another in the filesystem, search for text in the file system, move something to the trash in the filesystem, display filesystem disk usage, read a line from the terminal, write something to the terminal, and retrieve process arguments.

The host language shall be TinyCC, and the compilation output shall be a console program.
