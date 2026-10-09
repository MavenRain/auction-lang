# auction-lang status

The build implements milestone M0 in the slices A1 to A6. It also has the
core operations of the design (milestone M1 in `SPEC.md`). M0 is not
complete: the open items O1 to O3 of `SPEC.md` need a ruling first. M2 and
M3 are not started.

## Implemented

- The type formers F1 to F15 on the tcc-json host (`formers/tcc-json.md`).
- The built-in types `Nat`, `Flag`, `Fin n`, `Rat` and `Matrix m n`, and
  their operations (`src/front/check.c:99-174`).
- `domain/finstoch.lang`: 10 definitions. The structure maps of FinStoch.
- `domain/auction.lang`: 81 definitions. Bid strategies, utilities,
  mechanisms, expected revenue, expected utility and the envelope objects.
- `domain/opengame.lang`: 90 definitions. The `OpenGame` family, bidders,
  the 2-bidder and 3-bidder auction games, their functions and the
  Bayes-Nash pipeline expectations.
- `domain/domain.lang`: 5 definitions and 4 families. The sample domain of
  lang-template.
- 20 example programs in `examples/`.
- The verbs `check`, `eval` and `build` of `build/langc`. `build` writes one
  JSON document (`docs/host/README.md`).
- The gate `make check` (`docs/VALIDATION.md`).

## Remaining M0 work

1. O1: keep or remove the 9 eval lines of `examples/bayesnash.lang`. This
   needs a ruling first. Without the lines, the gate has 300 evals.
2. O2: put the domain design in `design/DESIGN.md`. This needs a ruling on
   the design text first. The source is auction-cat.
3. O3: fill `probe/CAPABILITY.md`. This needs a host probe run first.

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
- A matrix has at most 1 << 24 rows or columns and 1 << 24 cells
  (`src/front/eval.c:1100-1101`). A larger matrix refuses with
  `EVAL_MATRIX_SIZE` (`test/emit/mat-size.lang`). The limit also applies
  to intermediate matrices. Ordinary 3-bidder mechanisms work at n = 3
  and n = 5; `test/emit/auction3-size.lang` checks their refusal at n = 12.
  The current 3-bidder open-game construction (`auctionGame3`) exceeds
  the limit at n = 3; its score kernels run at n = 2 (`SPEC.md` O4).
- The JSON `type` field of an instance is at most 4096 bytes.

## Internal boundaries

`src/front/` is the front end. `lexer.c` and `parser.c` read the source.
`check.c` holds the built-in table, the reserved names and the allow-list
refusals, and it checks the types. `eval.c` evaluates a term. `core.h` holds
the shared types and the evaluation limits.

`src/json.c` writes the JSON document of `build`. `src/main.c` holds the
command line verbs and the arena. The Makefile joins the 4 domain files into
`build/domain.c` with `gen/embed.c` (`Makefile:15`). Thus each domain name is
a core name.

`test/gate.sh` runs the tests. `test/parse/expect.txt` lists the parse
refusals. `test/check/expect.txt` lists the check refusals.
`test/emit/expect.txt` lists the build refusals. `test/eval/expect.txt`
lists the eval lines. `test/json/` holds the JSON golden of each example.

A program cannot declare a family (rule R1, `REFUSE_DATA`). It can use core
names, but cannot declare a definition with a core name (rule R4,
`REFUSE_NAME`). It cannot use a name of the allow-list refusals
(`REFUSE_ALLOW`). It cannot use text (`LEX_CHAR`).
