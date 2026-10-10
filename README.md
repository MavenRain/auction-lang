# auction-lang

auction-lang is a small checked language for auctions over finite values.
It ports the auction results of auction-cat (Lean 4) to programs. The
compiler checks a program, evaluates it and writes JSON. It also reads
that JSON again, and a program can use the instances of a read document.

The host is the TinyCC JSON host kit of lang-template. The compiler is
`build/langc`, a C99 program.

## Quick start

Build the compiler and run the gate:

```
make check
```

Check a program. The output is `ok`.

```
build/langc check examples/dominance.lang
```

Evaluate one definition. The output is `1`.

```
build/langc eval examples/utility.lang threeVickAbove3
```

Each line of `test/eval/expect.txt` gives one such call and its result.

Write the JSON document of a program:

```
build/langc build examples/dominance.lang
```

The output is:

```json
{"auction-lang":1,"instances":[{"name":"domRes2","type":"Flag","value":true}]}
```

Read a JSON document again. Each instance prints as one line
`name : type = value`. The output is `domRes2 : Flag = 1`.

```
build/langc read test/json/dominance.json
```

Use the instances of a document in a program (a query). Each instance
becomes a definition before the definitions of the program. The output is
`4/9`.

```
build/langc eval test/query/matrix.lang --read test/json/matrix.json corner
```

Each line of `test/query/expect.txt` gives one such query and its result.

## Files

- `SPEC.md`: the specification.
- `docs/STATUS.md`: what works, the remaining work and the known limits.
- `docs/VALIDATION.md`: the gate results for each slice.
- `docs/host/README.md`: the commands, the JSON encoding and the build of
  the host kit.
- `docs/host/CAPABILITY.md`: the host facts and limits, each with its test.
- `domain/`: the domain files. Each name in them is a core name.
- `examples/`: 22 example programs.
- `test/`: the test tables and the gate, `test/gate.sh`.
- `formers/FORMERS.md` and `formers/tcc-json.md`: the type formers and their
  status on this host.
- `probe/CAPABILITY.md`: the host probe answers for this language.
- `design/DESIGN.md`: the domain design, from auction-cat.

## Requirements

TinyCC, a C compiler `cc`, POSIX `sh`, `awk`, `cmp` and Node. No network
access is necessary.

## License

MIT OR Apache-2.0. See `LICENSE-MIT` and `LICENSE-APACHE`.
