# auction-lang status

The build implements milestone M0 in the slices A1 to A6 and the post-A6
commit. It also has the core operations of the design (milestone M1 in
`SPEC.md`), and the slices B1 to B4 add their tests. Milestone M2 (the
slices C1 to C4) adds the JSON reader, the typed read and the queries over
the instances of a read document. M0, M1 and M2 are done. `SPEC.md`
section 9 records the rulings on O1 to O12. M3 is not started.

## Implemented

- The type formers F1 to F15 on the tcc-json host (`formers/tcc-json.md`).
- The built-in types `Nat`, `Flag`, `Fin n`, `Rat` and `Matrix m n`, and
  their operations (`src/front/check.c:99-174`).
- `domain/finstoch.lang`: 10 definitions. The structure maps of FinStoch.
- `domain/auction.lang`: 81 definitions. Bid strategies, utilities,
  mechanisms, expected revenue, expected utility and the envelope objects.
- `domain/opengame.lang`: 94 definitions. The `OpenGame` family, bidders,
  the 2-bidder and 3-bidder auction games, their functions and the
  Bayes-Nash pipeline expectations.
- `domain/domain.lang`: 5 definitions and 4 families. The sample domain of
  lang-template.
- 22 example programs in `examples/`.
- The verbs `check`, `eval`, `build` and `read` of `build/langc`. `build`
  writes one JSON document. `read` reads that document and checks each
  value against its type. With `--read DOC`, `eval` and `build` make each
  instance of the document a definition of the program
  (`docs/host/README.md`).
- The gate `make check` (`docs/VALIDATION.md`).

## Remaining work

No M0, M1 or M2 work remains. `SPEC.md` section 9 records the rulings on
O1 to O12, and the rulings on O1 to O11 are done. `SPEC.md` section 10
lists the slices of M0, M1 and M2. M3 (hardening: speed, limits, a
checked certificate for the output) is not started, and no plan for it
exists. M3 holds these items:

- The ruling on O12 moves two choices of slice B4 to M3: the kron of an
  identity and the mechanism in the 3-bidder scores, and the kernels that
  have eval lines at n = 2 only.
- The gate time: 43 s for C1, 147 s for C2 on a quiet machine and 85 s
  for C3 at load 12 to 18. The reads section of `test/gate.sh` calls
  `langc eval` 254 times, and each call checks the example again.
- The print buffer of a read line (`READ_PRINT_MAX`, 64 KiB,
  `src/read.c:274`). The value of a Matrix line has no such limit since
  slice C2. M3 finds the lines that keep the limit.

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
- All memory comes from one arena of 1 GiB (`src/main.c:10`). A source file
  is at most 1 MiB (`src/front/front.h:7`).
- A matrix has at most 1 << 25 rows or columns and 1 << 25 cells
  (`src/front/eval.c:1100-1101`; `SPEC.md` O4, RULED 2026-10-08). A
  larger matrix refuses with `EVAL_MATRIX_SIZE`
  (`test/emit/mat-size.lang`). The limit also applies to intermediate
  matrices. Ordinary 3-bidder mechanisms work at n = 3 and n = 5;
  `test/emit/auction3-size.lang` checks their refusal at n = 13. The
  3-bidder open-game scores use the factored games of slice B4, so they
  work at n = 3; `test/emit/opengame-size.lang` checks their refusal at
  n = 4. The kron form of `auctionGame3` exceeds the limit at n = 3.
- The JSON `type` field of an instance is at most 4096 bytes.

## Internal boundaries

`src/front/` is the front end. `lexer.c` and `parser.c` read the source.
`check.c` holds the built-in table, the reserved names and the allow-list
refusals, and it checks the types. `eval.c` evaluates a term. `core.h` holds
the shared types and the evaluation limits.

`src/json.c` writes the JSON document of `build`. `src/read.c` reads that
document again for `read` and for `--read`. `src/main.c` holds the
command line verbs and the arena. The Makefile joins the 4 domain files into
`build/domain.c` with `gen/embed.c` (`Makefile:15`). Thus each domain name is
a core name.

`test/gate.sh` runs the tests. `test/parse/expect.txt` lists the parse
refusals. `test/check/expect.txt` lists the check refusals.
`test/emit/expect.txt` lists the build refusals. `test/eval/expect.txt`
lists the eval lines. `test/json/` holds the JSON golden of each example.
`test/read/expect.txt` lists the read refusals. `test/query/expect.txt`
lists the queries. `test/read-typed.c` holds the typed reader
regressions.

A program cannot declare a family (rule R1, `REFUSE_DATA`). It can use core
names, but cannot declare a definition with a core name (rule R4,
`REFUSE_NAME`). It cannot use a name of the allow-list refusals
(`REFUSE_ALLOW`). It cannot use text (`LEX_CHAR`).
