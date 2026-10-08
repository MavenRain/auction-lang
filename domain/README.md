# Domain interface

Replace the sample domain in `domain.lang` with the design of your
language. `finstoch.lang` holds the matrix definitions of slice A2
(matId, matCopy, matDiscard, matBraiding, the unitors and the
associators). `auction.lang` holds the auction library of slice A3
(strategies, utilities, mechanisms, expected revenue and the Vickrey
envelope). `opengame.lang` holds the open games
of slice A4: the OpenGame family, the bidders, the auction games and
their kernel functions. The Makefile joins `domain.lang`,
`finstoch.lang`, `auction.lang` and `opengame.lang` into `build/domain-all.lang`, and `gen/embed.c` embeds
that file in `build/langc` at build time. The definitions of the four
files have DOMAIN origin. Program files cannot redefine them. A message
about a domain definition names `domain/domain.lang` for all four files
(`src/front/front.c:13`), and its line is a line of
`build/domain-all.lang`.

`domain.lang` uses the program syntax, and it can also declare families:

```
family Color := red | green | blue
family Item := makeItem (itemColor : Color) (itemPrice : Nat) (itemCount : Nat)
family Lot (size : Nat) := makeLot (lotPrice : Nat)
def lineTotal : (item : Item) -> Nat :=
  fun (item : Item) => natMul (itemPrice item) (itemCount item)
```

- Only a domain file can declare a family (rule R1). A program file that
  declares one is refused.
- Each family, constructor, field and definition name in a domain file
  becomes a core name (rule R4). A program cannot reuse it.
- A field name is also its projection: `itemPrice item`.
- `fold` takes one case for each constructor, in declaration order.
- A family can have parameters, which give an indexed family (F15):
  `Lot 3`.
- Domain definitions are not instances. They do not appear in the JSON
  output of `langc build`.

The JSON form of a family value comes from its declaration: a string for
a family whose constructors have no fields, an object of fields for a
family with one constructor, and an object with a `tag` field for any
other family. Do not use `tag` as a field name in a family with two or
more constructors.

After a change, update `examples/`, `test/eval/expect.txt` and the
goldens in `test/json/`, then run `make check`.
