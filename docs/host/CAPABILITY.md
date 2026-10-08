# TinyCC JSON host capability

This file holds host facts only. Put the facts of your domain in the
`probe/CAPABILITY.md` of your language. Paths are relative to the host
kit, or to the root of a generated language.

## Toolchain

- TinyCC 0.9.28rc (AArch64 Darwin) builds `build/langc` with
  `-std=c99 -Wall -Werror` (`Makefile`).
- `cc -Wall -Wextra -Wswitch-enum -Werror -fsyntax-only` checks the same
  sources (`make check-clang`). Each switch on an enum names every case
  and has no default arm.
- The Makefile joins `domain/domain.lang`, `domain/finstoch.lang` and
  `domain/auction.lang` into `build/domain-all.lang`. `tcc -run
  gen/embed.c` turns that file into `build/domain.c` at build time. The
  domain is part of the executable. A message about a domain definition
  names `domain/domain.lang` for all three files
  (`src/front/front.c:13`), and its line is a line of
  `build/domain-all.lang`.

## Values

- `Nat` is an unsigned 64-bit integer. Literals larger than
  18446744073709551615 are refused by the lexer (`test/parse/nat-range.lang`).
- `natAdd` and `natMul` trap on overflow. `natSub` is truncated at 0.
  A trap is a value: it stops a computation only when the computation
  needs it. `langc eval` prints `trap` (`test/eval/expect.txt`).
  `langc build` refuses an instance that holds a trap (`EVAL_OVERFLOW`, `EVAL_DIV_ZERO` for a division by zero, and `EVAL_STOCHASTIC` or `EVAL_MATRIX_SIZE` for a matrix).
- `Fin n` is an index below `n`. Its value is a Nat. A literal at the
  type `Fin n` needs a number `n` above the literal. The operations are
  finVal, finPair, finFirst, finSecond, finSub (truncated at 0), finMax,
  finEq, finLt, finLe and allFin. allFin n f applies f to 0, 1, ...,
  n - 1 and stops at the first flagNo or trap. allFin 0 f is flagYes.
- `Rat` is a fraction in lowest terms: a signed 64-bit numerator above
  -2^63 and an unsigned 64-bit denominator above 0. Zero is 0/1. A
  literal is `N/D` with no spaces (`1/3`). The checker reduces a literal
  to lowest terms (`2/6` is `1/3`). `N/0` is `TYPE_RAT_ZERO`, and a
  numerator above 2^63 - 1 after the reduction is `TYPE_RAT_RANGE`. A
  plain `N` is a Nat. There is no negative literal: write `ratSub 0/1 x`.
  The operations are ratAdd, ratSub, ratMul, ratDiv, ratOfNat, ratEq,
  ratLe, ratLt and sumRat. The arithmetic is exact. A result that does
  not fit is a trap (`EVAL_OVERFLOW`), ratOfNat of a Nat above 2^63 - 1
  is a trap (`EVAL_OVERFLOW`), and ratDiv by 0/1 is a trap
  (`EVAL_DIV_ZERO`). sumRat n f is f 0 + (f 1 + (... + (f (n - 1) +
  0/1))). It adds from f (n - 1) down to f 0 and stops at the first
  trap. sumRat 0 f is 0/1. `langc eval` prints a Rat as `N/D`.
- `Matrix m n` is a stochastic matrix with m rows and n columns
  (FinStoch.lean:433). Each cell is a Rat. In each row, each cell is
  0/1 or more and the sum of the cells is exactly 1/1. The operations
  are matTabulate, matOfFn, matEntry, matComp, matKron and matEq.
  matTabulate m n f makes the cells f i j and checks each row: a cell
  below 0/1 or a row sum other than 1/1 is a trap (`EVAL_STOCHASTIC`).
  matOfFn m n f puts 1/1 in column f i of row i and 0/1 in the other
  columns. matComp m k n M N is the matrix product. matKron m k m' k'
  M N is the Kronecker product, with natMul m m' rows and natMul k k'
  columns: cell (x, y) is M (x mod m) (y mod k) times N (x div m)
  (y div k). The first component of a Fin (natMul x y) index is the
  low digit. matEntry M i j is a cell, and matEq M N compares all the
  cells. A matrix with more than 2^24 rows, columns or cells is a trap
  (`EVAL_MATRIX_SIZE`). A cell that does not fit is a trap
  (`EVAL_OVERFLOW`). `domain/finstoch.lang` defines matId, matCopy,
  matDiscard, matBraiding, matLeftUnitor, matRightUnitor,
  matLeftUnitorInv, matRightUnitorInv, matAssociator and
  matAssociatorInv with matOfFn. `langc eval` prints a matrix as
  `[[1/1, 0/1], [0/1, 1/1]]`, and a matrix with 0 rows as `[]`.
- `domain/auction.lang` is the auction library (slice A3). It defines
  the bid strategies truthful and halfShading, the bidder utilities for
  2 and 3 bidders, the mechanisms secondPriceSealedBid,
  firstPriceSealedBid, spsbReserve, fpsbReserve, their 3-bidder forms,
  dutchAuction and englishAuction, biddedMechanism, expectedRevenue and
  uniformPrior (and the 3-bidder forms), the expected utilities, and the
  Vickrey allocation, payment and envelope definitions. A 2-bidder
  mechanism for n values is a matrix with natMul n n rows and natMul
  (natMul 2 n) (natMul 2 n) columns. A column is an outcome: for each
  bidder, a win index (Fin 2) and a price (Fin n). All 81 definitions,
  the At and Win helpers too, are core names
  (`test/check/auction-name.lang`, `test/check/auction-helper-name.lang`).
  The examples give a strategy or a prior as a lambda, for example
  `(fun (v : Fin 3) => halfShading 3 v)` (`examples/auction.lang`).
  secondPriceSealedBid 46 has more than 2^24 cells, a trap
  (`EVAL_MATRIX_SIZE`, `test/emit/auction-size.lang`).
- `flagIf x a b` is the if-then-else at any result type.
- `langc check` refuses these names with `REFUSE_ALLOW`, and the message
  gives the allowed form: natMin, natGe, natGt, flagOr, finMin, finGe,
  finGt, finAdd, finMul, ratMin, ratMax, ratAbs, ratNeg, ratInv, ratGe,
  ratGt, ratFloor, ratCeil, matMul, matTensor, matDet, matTranspose,
  matInv, matAdd and matScale. The lexer refuses text (`test/parse/string.lang`).
- There is no text type. The lexer refuses `"` (`test/parse/string.lang`).
- Values carry no types. The JSON writer gets each type from the
  definition type and from the family declarations.

## Limits

- The checker and the evaluator stop at a depth of 2000
  (`src/front/check.c:10`, `src/front/core.h:16`), and the parser at a
  nesting of 1000 (`src/front/parser.c:16`). These limits keep the C stack
  bounded. `test/gate.sh` checks a nesting of 1100 parentheses.
- The evaluator has a fuel of 20,000,000 steps for each instance
  (`src/front/core.h:17`). `langc build` starts the fuel again for each
  instance.
- All memory comes from one arena with a limit of 1 GiB (`src/main.c:10`).
  A run that reaches it stops with `OOM`.
- Source files are at most 1 MiB (`src/front/front.h:7`).

## Output

- `langc build` makes the full JSON document in the arena, then writes
  it. A refused build writes no partial document and no `-o` file.
- The writer walks list spines in a loop, so a long list does not use C
  stack. Other nesting counts against the JSON depth of 2000
  (`src/json.h:15`).
