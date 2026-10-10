# auction-lang status

The build implements milestone M0 in the slices A1 to A6 and the post-A6
commit. It also has the core operations of the design (milestone M1 in
`SPEC.md`), and the slices B1 to B4 add their tests. Milestone M2 (the
slices C1 to C4) adds the JSON reader, the typed read and the queries over
the instances of a read document. M0, M1 and M2 are done. `SPEC.md`
section 9 records the rulings on O1 to O17. M3 is in progress: slices D1
to D5 are done.

## Implemented

- The type formers F1 to F15 on the tcc-json host (`formers/tcc-json.md`).
- The built-in types `Nat`, `Flag`, `Fin n`, `Rat` and `Matrix m n`, and
  their operations (`src/front/check.c:99-175`).
- The core operation `matIdKronComp` (slice D2): the composition with the
  kron of an identity, with no kron. `gameScore` uses it.
- `domain/finstoch.lang`: 10 definitions. The structure maps of FinStoch.
- `domain/auction.lang`: 81 definitions. Bid strategies, utilities,
  mechanisms, expected revenue, expected utility and the envelope objects.
- `domain/opengame.lang`: 94 definitions. The `OpenGame` family, bidders,
  the 2-bidder and 3-bidder auction games, their functions and the
  Bayes-Nash pipeline expectations.
- `domain/domain.lang`: 5 definitions and 4 families. The sample domain of
  lang-template.
- 22 example programs in `examples/`.
- The verbs `check`, `eval`, `build`, `read` and `verify` of `build/langc`.
  `build` writes one JSON document. `read` reads that document and checks
  each value against its type. With `--read DOC`, `eval` and `build` make
  each instance of the document a definition of the program
  (`docs/host/README.md`).
- The verb `verify PROG DOC` (slice D3): a checked certificate for the
  output. It compares each instance of DOC with the value of that instance
  in PROG, and the bytes of DOC with the bytes that `build PROG` writes
  (`src/main.c:165`, `src/read.c:879`). A difference is a refusal with
  exit 1: `VERIFY_MISSING`, `VERIFY_VALUE`, `VERIFY_EXTRA` or
  `VERIFY_BYTES`.
- Full lines (slice D4): `read` and `eval` print each line in full.
  `value_print_len` and `value_text` (`src/front/eval.c:2218,2236`) count
  the text of a value, then print it into an arena buffer of that size.
  The writer and the reader refuse a JSON `type` longer than 4096 bytes
  with exit 1: `JSON_TYPE_SIZE` (`src/json.c:385-387`) and
  `READ_TYPE_SIZE` (`src/read.c:784`).
- The gate `make check` (`docs/VALIDATION.md`).

## Remaining work

No M0, M1 or M2 work remains. `SPEC.md` section 9 records the rulings on
O1 to O17, and the rulings on O1 to O11 are done. O13 to O17 are the
items of M3 (hardening: speed, limits, a checked certificate for the
output), and the rulings of 2026-10-09 accept each proposal. `SPEC.md`
section 10 lists the slices of M0, M1 and M2 and the slices D1 to D6 of
M3. Slice D1 is done: `matComp` skips a term when its left cell is 0, so
the 3 slowest eval lines take 3.2 s in place of 19.3 s. Slice D2 is
done: with the core operation `matIdKronComp`, the 3-bidder scores run
at n = 4, and the 3 slowest eval lines of D1 take 0.1 s each. Slice D3
is done: the verb `langc verify PROG DOC` checks a document against its
program. The reads section of `test/gate.sh` calls it once for each
document, in place of 254 calls of `langc eval`, so the reads loop takes
2.2 s in place of 9.5 s before review. All 254 read values also match
no-argument expectations checked independently by the eval group;
review adds the 46 missing expectations. Slice D4 is done: `read` and
`eval` print each line in full (before D4, a line longer than 64 KiB
printed with a "..." cut). The writer and the reader refuse a `type`
longer than 4096 bytes, and each boundary build that passes reads back.
Slice D5 is done: the 6 kernels of O16 have eval lines at n = 3, and each
line takes 0.09 s alone. The remaining slice follows the rulings:

- D6: close M3, docs only.

## Known limits

- `Nat` is an unsigned 64-bit value. `natAdd` and `natMul` trap on
  overflow: `EVAL_OVERFLOW` (`test/emit/trap.lang`). `natSub` stops at 0.
- A division by 0 traps: `EVAL_DIV_ZERO` (`test/emit/divzero.lang`).
- `Rat` has a numerator above -2^63 and below 2^63 and a positive
  unsigned 64-bit denominator. Arithmetic that does not fit traps with
  `EVAL_OVERFLOW` (`docs/host/CAPABILITY.md`).
- Depth limits keep the C stack bounded. The parser stops at a nesting of
  1000 (`src/front/parser.c:16`). The checker stops at a depth of 2000
  (`src/front/check.c:10`). The evaluator stops at a depth of 2000
  (`src/front/core.h:17`). The JSON writer stops at a depth of 2000
  (`src/json.h:15`). No test in `test/` reaches the JSON depth limit.
- The evaluator has a fuel of 20,000,000 steps for each instance
  (`src/front/core.h:18`). No test in `test/` reaches the fuel limit.
- All memory comes from one arena of 1 GiB (`src/main.c:11`). A source file
  is at most 1 MiB (`src/front/front.h:7`).
- A matrix has at most 1 << 25 rows or columns and 1 << 25 cells
  (`src/front/eval.c:1100-1101`; `SPEC.md` O4, RULED 2026-10-08). A
  larger matrix refuses with `EVAL_MATRIX_SIZE`
  (`test/emit/mat-size.lang`). The limit also applies to intermediate
  matrices. Ordinary 3-bidder mechanisms work at n = 3 and n = 5;
  `test/emit/auction3-size.lang` checks their refusal at n = 13. The
  3-bidder open-game scores use the factored games of slice B4 and
  `matIdKronComp` of slice D2, so they work at n = 4. At n = 5 the size
  count fits, but the evaluation stops at the arena limit (`langc: OOM`,
  exit 1). `test/emit/opengame-size.lang` checks their refusal at n = 6.
  The kron form of `auctionGame3` exceeds the limit at n = 3.
- The JSON `type` field of an instance is at most 4096 bytes
  (`src/json.c:10`, `src/read.c:274`). The writer refuses a longer type
  with `JSON_TYPE_SIZE`, and the reader refuses it with `READ_TYPE_SIZE`
  (exit 1). The writer also refuses a type that nests 200 levels, because
  the printer cuts it (`JSON_TYPE_SIZE`). The gate group `limits` checks
  types of 4096 and 4097 bytes and of 199 and 200 levels.
- A value line of `read` or `eval` has no size limit, but the printer
  stops at 200 nested levels (`src/front/eval.c:7`). The printer prints
  the last argument of each operation in a loop, so a long list does not
  add levels. A value that nests 200 levels in the other arguments prints
  with a "..." cut and exit 0, for example `pair (pair (pair 0 0) 0) 0`
  with 200 levels of `pair`. D4 keeps this cut (`SPEC.md` O14).

## Internal boundaries

`src/front/` is the front end. `lexer.c` and `parser.c` read the source.
`check.c` holds the built-in table, the reserved names and the allow-list
refusals, and it checks the types. `eval.c` evaluates a term. `core.h` holds
the shared types and the evaluation limits.

`src/json.c` writes the JSON document of `build`. `src/read.c` reads that
document again for `read`, `--read` and `verify`. `src/main.c` holds the
command line verbs and the arena. The Makefile joins the 4 domain files into
`build/domain.c` with `gen/embed.c` (`Makefile:15`). Thus each domain name is
a core name.

`test/gate.sh` runs the tests. `test/parse/expect.txt` lists the parse
refusals. `test/check/expect.txt` lists the check refusals.
`test/emit/expect.txt` lists the build refusals. `test/eval/expect.txt`
lists the eval lines. `test/json/` holds the JSON golden of each example.
`test/read/expect.txt` lists the read refusals. `test/query/expect.txt`
lists the queries. `test/verify/expect.txt` lists the verify refusals.
`test/read-typed.c` holds the typed reader regressions.

A program cannot declare a family (rule R1, `REFUSE_DATA`). It can use core
names, but cannot declare a definition with a core name (rule R4,
`REFUSE_NAME`). It cannot use a name of the allow-list refusals
(`REFUSE_ALLOW`). It cannot use text (`LEX_CHAR`).
