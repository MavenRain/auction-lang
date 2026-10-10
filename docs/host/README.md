# TinyCC JSON host kit

This kit compiles a small typed language to JSON. A C front end (lexer,
parser, checker and evaluator) and a JSON writer build with TinyCC into
one executable, `build/langc`. The output is the final state of each
instance in the program. There is no IR and no lowering step.

From this kit directory, or from a generated language directory:

```sh
make check
build/langc check examples/inventory.lang
build/langc eval examples/inventory.lang stockTotal
build/langc build examples/inventory.lang -o inventory.json
```

The build uses TinyCC (`TCC`, default `tcc`). Validation uses TinyCC
0.9.28rc on AArch64 Darwin. `make check-clang` also compiles the sources
with `cc -Wall -Wextra -Wswitch-enum -Werror -fsyntax-only` (`CC`, default
`cc`). The gate uses POSIX `sh`, `awk`, `cmp` and Node (for `JSON.parse`).
No network access or package installation is needed.

## Commands

| Command | Result |
|---|---|
| `langc check PROG` | Checks the program. Prints `ok`. |
| `langc eval PROG NAME [ARGS...]` | Applies the definition `NAME` to the literal arguments and prints the normal form. An overflow or a division by zero prints `trap`. |
| `langc build PROG [-o OUT]` | Checks the program, evaluates each instance and writes one JSON document to stdout, or to `OUT` |
| `langc eval PROG --read DOC NAME [ARGS...]` | Reads `DOC` as `langc read DOC` does, then checks the program and applies its definition `NAME`. Each instance of `DOC` is a definition of the program (see Queries). `--read DOC` comes right after `PROG` |
| `langc build PROG --read DOC [-o OUT]` | Reads `DOC`, then builds the program. The output document holds the instances of the program only, not the instances of `DOC` |
| `langc read [--json] DOC` | Reads a JSON document of `langc build` (format version 1). It parses and checks the `type` of each instance, decodes the `value` against that type, and prints one line `name : type = value` for each instance. The value prints as in `langc eval`. With `--json`, it does not check the types, and it writes the document again to stdout, in the format of `langc build` |
| `langc verify PROG DOC` | Checks the program, evaluates each instance once and compares `DOC` with it: each instance of the program is in `DOC` with the same type and value, `DOC` has no other instance, and the bytes of `DOC` are the bytes that `langc build PROG` writes. Then it prints one line `name : type = value` for each instance, as `langc read DOC` does |

Exit 0 is success. Exit 1 is a refused program. Exit 2 is a usage or IO
error. A refusal writes one line to stderr: `langc: CODE: NAME: message`.
`langc build` makes the full document in memory before it writes. Thus a
refused build writes nothing to stdout and makes no `OUT` file.

## The JSON document

```json
{"auction-lang":1,"instances":[{"name":"stockTotal","type":"Nat","value":70}]}
```

The key is the language name and the value is the format version, 1.
`instances` holds one object for each instance, in source order. An
instance is a definition of the program (not of `domain/`) whose type is
data. These definitions are checked, but they are not instances:

- functions (a Pi type, dependent or not);
- types and families (a universe type);
- equality proofs (an `Eq` type);
- the definitions in `domain/domain.lang`, `domain/finstoch.lang`,
  `domain/auction.lang` and `domain/opengame.lang`.

`type` is the normal form of the definition type in source syntax, for
example `Sigma (n : Nat) (Eq Nat n 1200)`. It is at most 4096 bytes.
`value` is the evaluated value. Values carry no types, so the writer walks
the type and the value together (`src/json.c`). The fuel of the evaluator
starts again for each instance.

| Type | JSON value |
|---|---|
| `Nat` | A number: the full unsigned 64-bit value in decimal |
| `Rat` | `{"num": n, "den": d}`, the fraction in lowest terms: `n` is a signed 64-bit value above -2^63 and `d` is above 0. Zero is `{"num": 0, "den": 1}`. |
| `Matrix m n` | An array of m rows. Each row is an array of n Rat values, for example `[[{"num":1,"den":2},{"num":1,"den":2}]]`. A matrix with 0 rows is `[]`. |
| `Flag` | `true` or `false` |
| `Unit` | `{}` |
| `Prod A B` | `{"first": a, "second": b}` |
| `Sum A B` | `{"inl": a}` or `{"inr": b}` |
| `Option A` | `null` or the value. When `A` is an `Option` or an `Eq` type, `some v` is `{"some": v}`. |
| `List A` | An array |
| `Sigma (x : A) B` | `{"witness": w, "payload": p}`. The payload type is `B` at the witness. |
| `Eq A x y` | `null` |
| A family whose constructors have no fields | The constructor name, as a string |
| A family with one constructor | An object of the fields |
| Any other family | `{"tag": "name", ...}` with the fields of that constructor |

An indexed family uses the same rules. Its index is in `type` (for
example `Lot 3`), not in `value`.

`langc build` refuses a program with these codes:

| Code | Cause |
|---|---|
| `JSON_VALUE` | An instance holds a function, a type or a stuck term |
| `JSON_DEPTH` | An instance nests deeper than 2000 levels. A list spine does not count. |
| `JSON_TYPE_SIZE` | The type of an instance has more than 4096 bytes, or it nests 200 levels, so the printer cannot print it in full. The writer checks the type before it evaluates the instance |
| `EVAL_OVERFLOW` | A Nat, Rat or Matrix operation in an instance overflows |
| `EVAL_DIV_ZERO` | A natDiv, natMod or ratDiv in an instance divides by zero |
| `EVAL_STOCHASTIC` | A matTabulate row in an instance has a cell below 0/1 or a sum other than 1/1 |
| `EVAL_MATRIX_SIZE` | A matrix in an instance has more than 2^25 rows, columns or cells |

The check and evaluation codes are the same as in `langc check` and
`langc eval`. `test/parse`, `test/check` and `test/emit` hold a program
for each refusal.

`langc read DOC` refuses a document with these codes:

| Code | Cause |
|---|---|
| `READ_SIZE` | The document is larger than 16 MiB |
| `READ_SYNTAX` | The document is not JSON text in ASCII with objects, arrays, strings, integers, `true`, `false` and `null`. The message gives the byte offset |
| `READ_DEPTH` | The document nests more than 2008 arrays and objects |
| `READ_VERSION` | The first member is not `"auction-lang":1` (format version 1) |
| `READ_SHAPE` | The second and last member is not `"instances"`, an array of objects with the members `name`, `type` and `value` in this order |
| `READ_NAME` | Two instances have the same name, or a name contains an ASCII control byte. With `--read` (queries), also a name that is not exactly one identifier, for example `a b` or the keyword `def` |
| `READ_TYPE` | A `type` text is not exactly one type that checks after the domain, or it is a function type, a universe or an equality |
| `READ_TYPE_SIZE` | A decoded `type` text has more than 4096 UTF-8 bytes (the NUL not counted). The reader checks this before it parses the type |
| `READ_VALUE` | A `value` does not match its type (the table above). For example: a Nat above 2^64-1, a `Fin n` value that is not below n, a Rat that is not in lowest terms, a Matrix with a wrong number of rows, a Matrix row that does not sum to 1, or `null` for an `Eq` type whose two sides are not equal |

Typed reads check each type against the domain in isolation and infer its
universe. With `--json`, only the first 5 codes apply. `test/read` holds a
document for each refusal, and `test/gate.sh` makes the large documents.
`test/read-typed.c` checks built documents with higher-universe types and
wide matrix sums, plus type isolation and name controls, in `make check`.

`langc verify PROG DOC` refuses with the codes of `langc check`,
`langc build` and `langc read DOC`, and with these codes. It prints
nothing on stdout when it refuses:

| Code | Cause |
|---|---|
| `VERIFY_MISSING` | An instance of the program is not in `DOC` |
| `VERIFY_VALUE` | The type or the value of an instance in `DOC` is not the type or the value of that instance in the program |
| `VERIFY_EXTRA` | An instance of `DOC` is not an instance of the program |
| `VERIFY_BYTES` | The instances agree, but the bytes of `DOC` are not the bytes that `langc build PROG` writes. The message gives the first byte that is different |

The checks run in this order: for each instance of the program, missing
and then value; then extra; then bytes. `test/verify` holds a document
for each code.

Queries: with `--read DOC`, `langc eval` and `langc build` read `DOC`
first. Each instance of `DOC` becomes a definition of the program, with
its read type and its read value, before the definitions of the program.
A query is a definition of the program that uses these names. An
instance name that is a core name, a domain definition or a name of the
program is `REFUSE_NAME` (rule R4 in `SPEC.md`). With `--read DOC`,
`langc build` writes the instances of the program only. `test/query` holds
the query programs, the refusal documents and `expect.txt`.

`langc check` refuses Fin n terms, Rat literals and names outside the allow-list with
these codes:

| Code | Cause |
|---|---|
| `TYPE_FIN_RANGE` | A literal at the type `Fin n` is not below `n` |
| `TYPE_FIN_OPEN` | A literal at the type `Fin n` where `n` is not a number |
| `TYPE_FIN_SIZE` | The size `natMul m n` of a finPair, finFirst or finSecond is not below 2^64 |
| `TYPE_RAT_ZERO` | A Rat literal `N/D` with `D` = 0 |
| `TYPE_RAT_RANGE` | The numerator of a Rat literal, after the reduction to lowest terms, is above 2^63 - 1 |
| `REFUSE_ALLOW` | A name outside the allow-list, for example `natMin`, `flagOr`, `finMin`, `ratNeg` or `matMul` |

`langc eval` refuses a Fin argument that is not below its size with
`EVAL_ARGS`. `langc eval` does not take a Rat argument. A Rat result
prints as `N/D`, for example `-1/6`. `langc eval` does not take a
Matrix argument. A Matrix result prints its rows in brackets, for
example `[[1/2, 1/2]]`.

## The domain

`domain/domain.lang` is the sample domain. `domain/finstoch.lang` holds
the matrix definitions (slice A2). `domain/auction.lang` holds the
auction library (slice A3). `domain/opengame.lang` holds the open
games and the auction kernels (slice A4). The Makefile joins the four
files into
`build/domain-all.lang`, and `gen/embed.c` embeds that file in the
executable at build time. `domain/README.md` tells how to replace it.

## Limits

| Limit | Value | Source |
|---|---|---|
| Source size | 1 MiB | `src/front/front.h:7` |
| Arena | 1 GiB | `src/main.c:11` |
| Parser nesting | 1000 | `src/front/parser.c:16` |
| Checker depth | 2000 | `src/front/check.c:10` |
| Evaluator depth | 2000 | `src/front/core.h:17` |
| Evaluator fuel, for each instance | 20,000,000 steps | `src/front/core.h:18` |
| JSON nesting | 2000 | `src/json.h:15` |
| Read document size | 16 MiB | `src/read.h:19` |
| Read nesting | 2008 | `src/read.h:24` |
| `type` text, writer and reader | 4096 bytes | `src/json.c:10`, `src/read.c:274` |
| Value line of `read` and `eval` | No size limit. A "..." cut at 200 nested levels | `src/front/eval.c:7` |

## Origin

This kit is a fork of the tcc-wasm kit front end. It does not have the
Wasm writer (`wasm.c`), the IR (`ir.h`) or the target interface
(`target.h`). It adds `src/json.c` and `src/json.h`, and `src/main.c` has
the verbs `check`, `eval` and `build`. At the fork, the files in `src/front/`
were the same bytes as in the snapshot below. Slice A1 changed some of them. To find the changes after the fork,
compare the hashes with `shasum -a 256 src/front/*` in the tcc-wasm kit.

SHA-256 of the tcc-wasm kit sources at the fork (paths relative to that
kit):

| Path | SHA-256 |
|---|---|
| `src/front/ast.h` | `5dbe74e54c8207da17a57810624bf049d729f272c73a0765b9fd504fb3f18720` |
| `src/front/base.c` | `d994f71d11a352896212d97ed6edb5de062dc37ad6f31bf927487c4d8e675d5b` |
| `src/front/base.h` | `d36daae15e9a79ae298eadf5f1f9c3687e077c7305e63fad6cc5b477b58cebca` |
| `src/front/check.c` | `526a8207c66f3276c18767c62de05026866e7e8adc30e2c5383a2975c0a57114` |
| `src/front/check.h` | `5d781eb3f52a7a17db10d375c0fb7c5b578bf0527dd358cfe49ebb779d008d06` |
| `src/front/core.h` | `d04aa9412bb0f25d470bd3b91e3a073284b19e89aa90083e18d63a6d468fd9dc` |
| `src/front/eval.c` | `5a044b8935f77614a2321883afc8df7d2b1d745e198b8f861538dc1a61773c55` |
| `src/front/front.c` | `9b7d8890d0854fe989780431521189fabf0a79e624359733f5a90822bbf08c27` |
| `src/front/front.h` | `7eb0b8de605ad7336eac1bffc9b65587d7113b2e3849a95a9e6adf75fd12989a` |
| `src/front/lexer.c` | `6812190d933fa5dfb1ad60c9d40a46aeb460aab5342feed6c4697fe31a2425bc` |
| `src/front/lexer.h` | `85046ffc1f76878b8dc0102db08458a59fef914d7951462c9e7bf590cb5db314` |
| `src/front/parser.c` | `6220dcc67e0fc4782d60cf9516bae979d2d4eb3c5eb3e047d92b85806b0bb02d` |
| `src/front/parser.h` | `e9caa8e0dbcfc716b572fa754daa3023ee1a55fb16ff77fb0235c97a49adaa1c` |
| `src/ir.h` | `13021f895667552ec1d527267258e8a3437cadde74744a348e1219dff6d773b9` |
| `src/main.c` | `2294beb677fb4c208811937bbf48f1570911de6e52a6068874e90b3489b211ba` |
| `src/target.h` | `077dff69391f69dbd4623858553dee7b5547a8c1edc0c75660e0aede9b15e429` |
| `src/wasm.c` | `4f69fba5ea1f335e95c7f030d970c4e89da84ee13c57d95879137cf96d8235bd` |

`FORMERS.md`, copied to `formers/tcc-json.md` in a generated language,
gives the status of each former. `docs/CAPABILITY.md` gives the host
facts.
