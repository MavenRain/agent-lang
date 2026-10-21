# Domain interface

Replace the sample domain in `domain.lang` with the design of your
language. `gen/embed.c` embeds this file in `build/langc` at build time.
Its definitions have DOMAIN origin. Program files cannot redefine them.

`domain.lang` uses the program syntax, and it can also declare families:

```
family Json := jnull | jbool (jboolValue : Bool) | jnum (jnumValue : Sum Int64 Text) | jstr (jstrValue : Text) | jarr (jarrItems : JsonList) | jobj (jobjFields : JsonFields)
and JsonList := jlNil | jlCons (jlHead : Json) (jlTail : JsonList)
and JsonFields := jfNil | jfCons (jfKey : Text) (jfValue : Json) (jfTail : JsonFields)
family Log := logEmpty | logAppend (logRest : Log) (logEntry : Json)
def logLength : (l : Log) -> Nat :=
  fun (l : Log) => fold 0 (fun (rest : Nat) (entry : Json) => natAdd rest 1) l
```

- Only `domain.lang` can declare a family (rule R1). A program file that
  declares one is refused.
- Each family, constructor, field and definition name in `domain.lang`
  becomes a core name (rule R4). A program cannot reuse it.
- Constructor fields must have distinct names and cannot reuse their
  constructor's name. Collisions are refused with `REFUSE_NAME`.
- A field name is also its projection: `logEntry entry`.
- `fold` takes one case for each constructor, in declaration order.
- A family can have parameters, which give an indexed family (F15):
  `Vec Nat 3`.
- A family lives in the largest universe of its fields. For example,
  `family U := makeU (decode : Type 0)` has type `Type 1`.
- Domain definitions are not instances. They do not appear in the JSON
  output of `langc json`.

The JSON form of a family value comes from its declaration: a string for
a family whose constructors have no fields, an object of fields for a
family with one constructor, and an object with a `tag` field for any
other family. The checker refuses `tag` as a field name in a family with
two or more constructors (`DOMAIN_FAMILY`), because it would overwrite
the JSON constructor tag.

After a change, update `examples/`, `test/eval/expect.txt` and the
goldens in `test/json/`, then run `make check`.
