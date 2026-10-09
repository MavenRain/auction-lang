# auction-lang specification

Status: milestone M0, slices A1 to A6, 2026-10-08.

## 1. Purpose

A program describes auctions over finite values. It gives bids, bid
strategies, priors, mechanisms and open games. A mechanism is a stochastic
matrix of `Rat` values. The compiler checks the program. Then it evaluates
each instance and writes the final state of each instance as one JSON
document.

The host is the TinyCC JSON host kit (tcc-json) of lang-template. The
target is JSON. The type formers are F1 to F15 of `formers/FORMERS.md`. The
core types and the core operations are only those of sections 5 and 6: the
built-ins of `src/front/check.c` and the 186 definitions in `domain/`. They
port auction-cat (Lean 4). `design/DESIGN.md` does not hold the design yet
(open item O2).

## 2. Programs

A program is a sequence of definitions:

```
def NAME : TYPE := TERM
```

A line comment starts with `--`. A program needs no fixed first
definition.

The compiler refuses these forms in a program:

| Form | Rule | Code | Test |
|---|---|---|---|
| `family`: a new data type | R1 | `REFUSE_DATA` | `test/check/data.lang` |
| `axiom`: an unproved fact | R2 | `REFUSE_AXIOM` | `test/check/axiom.lang` |
| `def rec`: general recursion | R3 | `REFUSE_REC` | `test/check/rec.lang` |
| A definition with a core name | R4 | `REFUSE_NAME` | `test/check/core-name.lang` |
| A name outside the allow-list, for example `natMin` | | `REFUSE_ALLOW` | `test/check/natmin-use.lang` |
| Text, for example `"bolt"` | | `LEX_CHAR` | `test/parse/string.lang` |

The core names are the built-ins of `src/front/check.c`, the reserved names
`Bool` and `Text` (`src/front/check.c:180`), and each name in a domain file.
The Makefile joins `domain/domain.lang`, `domain/finstoch.lang`,
`domain/auction.lang` and `domain/opengame.lang` in this order
(`Makefile:15`). The allow-list refusals have 25 names. Each diagnostic
gives an allowed form or states that no such operation is provided
(`src/front/check.c:185-210`). For example, write
`flagIf (natLe x y) x y` for `natMin x y`.

Thus a program cannot add a data type, an unproved fact or general
recursion. Recursion comes only from `fold` and `unfold` (F6, F7). The
compiler writes the target. A program cannot.

## 3. Type formers

The type formers are F1 to F15 of `formers/FORMERS.md`. This language uses
the tcc-json column of the realization matrix. `formers/tcc-json.md` gives
the host form of each former. All 15 formers are DONE on tcc-json
(`formers/tcc-json.md:12-26`). The table gives the 2 formers with a
restriction.

| ID | Status on tcc-json | Effect on this language | Open item |
|---|---|---|---|
| F7 | DONE into List with a Nat step limit | An `unfold` stops at its step limit. It cannot make an infinite list. | None |
| F15 | DONE in `domain/` only (rule R1) | Only a domain file can declare a family, for example `OpenGame` in `domain/opengame.lang`. | None |

## 4. Structures

- **Monad**: `pure`, `map`, `bind` on Option, Sum E and List.
- **Algebra**: `fold` on Nat, List and each family. `unfold` into List,
  with a Nat step limit.
- **Filterable**: `filter` on Option and List.

The source is `formers/tcc-json.md:16-19`. `Matrix` is not a carrier of
these structures. The core operation `matComp` composes two matrices.

## 5. Core types

No definition is probe-forced, because `probe/CAPABILITY.md` is not filled
(O3). The host facts are in `docs/host/CAPABILITY.md`.

| Type | Meaning (design section) | Definition |
|---|---|---|
| `Nat` | A count, a size or an index | Built-in (`src/front/check.c:99`). An unsigned 64-bit value. |
| `Flag` | A yes or no decision: `flagYes` or `flagNo` | Built-in (`src/front/check.c:100`) |
| `Fin n` | A value below n: a bid, a valuation, a bidder or an outcome | Built-in (`src/front/check.c:147`) |
| `Rat` | An exact fraction: a probability, a utility or a revenue | Built-in (`src/front/check.c:158`). The numerator is above -2^63 and below 2^63; the denominator is a positive unsigned 64-bit value. |
| `Matrix m n` | A stochastic matrix of `Rat` values with m rows and n columns: a kernel of FinStoch (auction-cat `MarkovCat/FinStoch.lean:433`) | Built-in (`src/front/check.c:168`). Each entry is nonnegative and each row sums to exactly 1/1. `matTabulate` traps with `EVAL_STOCHASTIC` otherwise. |
| `OpenGame x s y r m` | An open game over FinStoch (auction-cat `OpenGamesCat/Basic.lean`) | Family `makeGame (gameView : Matrix x (natMul m y)) (gameUpdate : Matrix (natMul m r) s)` (`domain/opengame.lang:8`) |
| `Color`, `Item`, `Stack`, `Lot size` | The sample domain of lang-template. It has no auction-cat source. | Families (`domain/domain.lang:4-8`) |

## 6. Core operations

The built-ins are in `src/front/check.c:135-174`. The Type column gives the
type, or the number of arguments and the result type.

| Operation | Type | Meaning (design section) |
|---|---|---|
| `natAdd`, `natSub`, `natMul`, `natDiv`, `natMod`, `natMax` | `Nat -> Nat -> Nat` | Nat arithmetic. `natAdd` and `natMul` trap on overflow. `natSub` stops at 0. |
| `natEq`, `natLe`, `natLt` | `Nat -> Nat -> Flag` | Nat comparison |
| `flagIf` | 3 arguments | `flagIf c x y` is x if c is `flagYes`, and y if not |
| `flagAnd`, `flagNot` | 2 and 1 arguments, `Flag` | Flag logic |
| `finVal` | 1 argument, `Nat` | The value of a `Fin n` |
| `finPair`, `finFirst`, `finSecond` | 4, 3 and 3 arguments, `Fin` | A joint value of `Fin (natMul x y)` and its 2 parts. The first part is the low digit. |
| `finSub`, `finMax` | 2 arguments, `Fin` | Fin subtraction and maximum |
| `finEq`, `finLt`, `finLe` | 2 arguments, `Flag` | Fin comparison |
| `allFin` | 2 arguments, `Flag` | A predicate is true on each value of `Fin n` |
| `ratAdd`, `ratSub`, `ratMul`, `ratDiv` | `Rat -> Rat -> Rat` | Exact fraction arithmetic |
| `ratOfNat` | `Nat -> Rat` | The fraction of a Nat |
| `ratEq`, `ratLe`, `ratLt` | `Rat -> Rat -> Flag` | Rat comparison |
| `sumRat` | 2 arguments, `Rat` | The sum of a function over `Fin n` |
| `matTabulate`, `matOfFn` | 3 arguments, `Matrix` | A matrix from a function to `Rat`, or the deterministic kernel of a function on `Fin` |
| `matEntry` | 3 arguments, `Rat` | One entry of a matrix |
| `matComp`, `matKron` | 5 and 6 arguments, `Matrix` | Composition and Kronecker product |
| `matEq` | 2 arguments, `Flag` | Matrix equality |

An overflow or a division by 0 traps: `EVAL_OVERFLOW`
(`test/emit/trap.lang`) and `EVAL_DIV_ZERO` (`test/emit/divzero.lang`).

The domain files hold 186 definitions: `domain/domain.lang` 5,
`domain/finstoch.lang` 10, `domain/auction.lang` 81 and
`domain/opengame.lang` 90. The Type column gives the place of the
definitions.

| Operation | Type | Meaning (design section) |
|---|---|---|
| `matId`, `matCopy`, `matDiscard`, `matBraiding`, the unitors and their inverses, `matAssociator`, `matAssociatorInv` | See `domain/finstoch.lang` | The structure maps of FinStoch (auction-cat `MarkovCat/FinStoch.lean`) |
| `truthful`, `halfShading` | See `domain/auction.lang:8` | Bid strategies `Fin n -> Fin n` (auction-cat `Examples.lean:202`, `Bidder.lean:114`) |
| `vickreyUtility`, `fpsbUtility`, `fpsbReserveUtility`, `vickreyReserveUtility` and the forms for bidders 2 and 3 | See `domain/auction.lang:13` and `:36` | The utility of one bidder in the second price, first price and reserve auctions, for 2 and 3 bidders |
| `spsbFn`, `fpsbFn`, `spsbReserveFn`, `fpsbReserveFn`, `spsb3Fn`, `fpsb3Fn` and their `At` and `Win` helpers | See `domain/auction.lang:63` | The outcome of each bidder over the joint bid: `finPair 2 n allocation price` |
| `secondPriceSealedBid`, `firstPriceSealedBid`, `spsbReserve`, `fpsbReserve`, `dutchAuction`, `englishAuction`, the forms with suffix 3, `biddedMechanism` | See `domain/auction.lang` | The mechanisms as matrices (auction-cat `Dutch.lean:42`, `English.lean:41`, `Auction.lean:168`) |
| `outcomeRevenue`, `uniformPrior`, `expectedRevenue` and the forms with suffix 3 | See `domain/auction.lang:135` | Outcome revenue, uniform priors and expected revenue under the supplied prior (auction-cat `Revenue.lean:62-241`, `Revenue3.lean:34-65`) |
| `vickreyExpectedUtility`, `fpsbExpectedUtility`, `fpsbReserveExpectedUtility` and the forms for bidders 2 and 3 | See `domain/auction.lang:152` | The expected utility of one bidder (auction-cat `BayesNash.lean:56-669`) |
| `vickreyAllocation`, `vickreyExpectedPayment`, `vickreyEqUtility`, `vickreyEnvelopeIntegral`, `paymentFromAllocation` | See `domain/auction.lang` | The envelope objects of the Vickrey auction |
| `gameId`, `gameComp`, `gameScore` | See `domain/opengame.lang:9` | The game layer (auction-cat `OpenGamesCat/Basic.lean:50`, `:55`, `:68`) |
| `middleInterchangeFn`, `matMiddleInterchange`, `gameKron` | See `domain/opengame.lang:16` | The middle interchange (`Basic.lean:80`) and the Kronecker product of two games |
| `makeBidder`, `truthfulBidder`, `deviatorBidder`, `halfShadeBidder` | See `domain/opengame.lang:24` | A bidder as an open game (auction-cat `Bidder.lean:49`) |
| `auctionGame`, `auctionScore`, `spsbAuction`, `fpsbAuction`, the reserve forms and the deviator forms | See `domain/opengame.lang:34` | The 2-bidder auction games (auction-cat `Auction.lean:38-160`) |
| `auctionGame3`, `auctionScore3`, `spsb3Auction`, `fpsb3Auction`, the reserve forms and the deviator forms | See `domain/opengame.lang:72` | The 3-bidder auction games (auction-cat `Auction.lean:201-318`) |
| The `Fn` and `Util` forms, for example `spsbAuctionFn` and `auctionBidder1Util` | See `domain/opengame.lang:110` | The deterministic function of each auction score and game |
| `auctionExpectedBidder1Util`, `vickreyReserveExpectedUtility` and the forms for bidders 2 and 3 | See `domain/opengame.lang:176` | The Bayes-Nash pipeline expectations (auction-cat `BayesNashPipeline.lean`) |

## 7. Target and instance encoding

An instance is a definition of the program, not of `domain/`, whose type
is data (F14). The compiler checks these definitions, but they are not
instances: functions, types and families, equality proofs, and the
definitions of the 4 domain files. `langc build` writes one document. For
`examples/dominance.lang` it is:

```json
{"auction-lang":1,"instances":[{"name":"domRes2","type":"Flag","value":true}]}
```

Each instance has a `name`, a `type` and a `value`. The `type` field is at
most 4096 bytes. An index of a family is in `type`, not in `value`.

| Type | Encoding |
|---|---|
| `Nat` | A number: the full unsigned 64-bit value in decimal |
| `Fin n` | A number below n, with the size n recorded in `type` |
| `Rat` | `{"num": n, "den": d}`, the fraction in lowest terms |
| `Matrix m n` | An array of m rows of n `Rat` values. A matrix with 0 rows is `[]`. |
| `Flag` | `true` or `false` |
| Unit | `{}` |
| Product (F1) | `{"first": a, "second": b}` |
| Coproduct (F2) | `{"inl": a}` or `{"inr": b}` |
| Option (F3) | `null` or the value. It is `{"some": v}` when the value is an Option or an `Eq` proof. |
| List (F4) | An array |
| Sigma (F11) | `{"witness": w, "payload": p}` |
| `Eq` (F12) | `null` |
| A family with no fields | A string: the constructor name |
| A family with one constructor | An object of its fields |
| Another family | An object with a `tag` field and the fields |

The full table is in `docs/host/README.md`, section The JSON document. A
proof erases: an `Eq` value is `null`, and a Sigma payload of `Eq` type is
`null` (`formers/tcc-json.md:31`). A build refuses a value with no JSON
form: `JSON_VALUE` (`test/emit/function.lang`).

## 8. Host and target

- Host: the tcc-json kit of lang-template. Its front end is a fork of the
  tcc-wasm kit front end. `docs/host/README.md`, section Origin, gives the
  SHA-256 of each front-end file at the fork.
- Base: commit cdc754e (2026-10-07), made by
  `bin/new-lang.sh auction-lang tcc-json` in lang-template. There is no
  `PIN` file.
- Tools: TinyCC 0.9.28rc 2026-09-04 mob@0fb54300 (AArch64 Darwin) builds
  `build/langc` with `-std=c99 -Wall -Werror`. Apple clang 21.0.0 checks the
  sources with `-Wswitch-enum -Werror -fsyntax-only`. The gate also uses
  POSIX `sh`, `awk`, `cmp` and Node.
- Host facts that set the design (`docs/host/CAPABILITY.md`): `Nat` is an
  unsigned 64-bit value. `natAdd` and `natMul` trap on overflow. `natSub`
  stops at 0. The evaluator has a depth limit and a fuel limit. One arena
  holds all memory. Item O4 gives the matrix limit.
- Output check: `make check` builds each example and compares the output
  with its golden in `test/json/` (`cmp`). Node parses each golden with
  `JSON.parse`. There is no axiom report, because the target holds no
  proof.

## 9. Open items

- O1. The 9 eval lines of `examples/bayesnash.lang` in
  `test/eval/expect.txt`: keep them or remove them. OPEN. The lines stay
  until a ruling. Without them, the gate has 300 evals.
- O2. `design/DESIGN.md` does not hold the domain design. The design source
  is auction-cat. OPEN.
- O3. `probe/CAPABILITY.md` is the unfilled template. The host facts are in
  `docs/host/CAPABILITY.md`. OPEN.
- O4. The matrix limit: at most 1 << 24 rows or columns, and 1 << 24 cells
  (`src/front/eval.c:1100-1101`). A larger matrix refuses with
  `EVAL_MATRIX_SIZE` (`test/emit/mat-size.lang`,
  `test/emit/auction3-size.lang`). This limit applies to intermediate
  matrices too. Ordinary 3-bidder mechanisms work at n = 3 and n = 5;
  `test/emit/auction3-size.lang` checks their size refusal at n = 12.
  The current 3-bidder open-game construction (`auctionGame3`) exceeds
  the limit at n = 3; its score kernels run at n = 2. Host limit. OPEN.

## 10. Milestones

| Milestone | Content |
|---|---|
| M0 | Host probe (`probe/CAPABILITY.md`); the core types construct and check; refusal list; examples and tests |
| M1 | Core operations |
| M2 | Queries and reads, or the contract |
| M3 | Hardening: speed, limits, a checked certificate for the output |

M0 has these slices:

| Slice | Commit | Content |
|---|---|---|
| Base | cdc754e | Generate auction-lang from lang-template with the tcc-json host kit |
| A1 | 22698b8 | The primitives `Nat`, `Flag`, `Fin n` and `Rat` |
| A2 | 10cfe36 | The stochastic `Matrix` type and its operations (`domain/finstoch.lang`) |
| A3 | 3c35793 | The auction library (`domain/auction.lang`) and its examples |
| A4 | d9c699f | Open games and auction kernels (`domain/opengame.lang`) |
| A5 | 50ac45c | Utility, revenue and dominance examples |
| A6 | This commit | This file, `docs/STATUS.md`, `docs/VALIDATION.md`, `README.md`, and 2 line citations in `docs/host/CAPABILITY.md` |

Status 2026-10-08: slices A1 to A6 are done. They also give the core
operations of M1. M0 is not complete: O1 to O3 are open. M2 and M3 are not
started.
