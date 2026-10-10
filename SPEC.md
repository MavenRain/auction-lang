# auction-lang specification

Status: milestones M0, M1 and M2 are done, 2026-10-09 (section 10).

## 1. Purpose

A program describes auctions over finite values. It gives bids, bid
strategies, priors, mechanisms and open games. A mechanism is a stochastic
matrix of `Rat` values. The compiler checks the program. Then it evaluates
each instance and writes the final state of each instance as one JSON
document.

The host is the TinyCC JSON host kit (tcc-json) of lang-template. The
target is JSON. The type formers are F1 to F15 of `formers/FORMERS.md`. The
core types and the core operations are only those of sections 5 and 6: the
built-ins of `src/front/check.c` and the 190 definitions in `domain/`. They
port auction-cat (Lean 4). `design/DESIGN.md` gives the design.

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

No definition is probe-forced. The probe answers are in
`probe/CAPABILITY.md`, and the host facts are in `docs/host/CAPABILITY.md`.
A section number in the Meaning column is a section of `design/DESIGN.md`.

| Type | Meaning (design section) | Definition |
|---|---|---|
| `Nat` | A count, a size or an index (section 2) | Built-in (`src/front/check.c:99`). An unsigned 64-bit value. |
| `Flag` | A yes or no decision: `flagYes` or `flagNo` (section 2) | Built-in (`src/front/check.c:100`) |
| `Fin n` | A value below n: a bid, a valuation, a bidder or an outcome (section 2) | Built-in (`src/front/check.c:147`) |
| `Rat` | An exact fraction: a probability, a utility or a revenue (section 2) | Built-in (`src/front/check.c:158`). The numerator is above -2^63 and below 2^63; the denominator is a positive unsigned 64-bit value. |
| `Matrix m n` | A stochastic matrix of `Rat` values with m rows and n columns: a kernel of FinStoch (auction-cat `MarkovCat/FinStoch.lean:433`) (sections 2 and 3) | Built-in (`src/front/check.c:168`). Each entry is nonnegative and each row sums to exactly 1/1. `matTabulate` traps with `EVAL_STOCHASTIC` otherwise. |
| `OpenGame x s y r m` | An open game over FinStoch (auction-cat `OpenGamesCat/Basic.lean`) (sections 2 and 6) | Family `makeGame (gameView : Matrix x (natMul m y)) (gameUpdate : Matrix (natMul m r) s)` (`domain/opengame.lang:8`) |
| `Color`, `Item`, `Stack`, `Lot size` | The sample domain of lang-template. It has no auction-cat source. | Families (`domain/domain.lang:4-8`) |

## 6. Core operations

The built-ins are in `src/front/check.c:135-174`. The Type column gives the
type, or the number of arguments and the result type. A section number in
the Meaning column is a section of `design/DESIGN.md`.

| Operation | Type | Meaning (design section) |
|---|---|---|
| `natAdd`, `natSub`, `natMul`, `natDiv`, `natMod`, `natMax` | `Nat -> Nat -> Nat` | Nat arithmetic. `natAdd` and `natMul` trap on overflow. `natSub` stops at 0. (section 2) |
| `natEq`, `natLe`, `natLt` | `Nat -> Nat -> Flag` | Nat comparison (section 2) |
| `flagIf` | 3 arguments | `flagIf c x y` is x if c is `flagYes`, and y if not (section 2) |
| `flagAnd`, `flagNot` | 2 and 1 arguments, `Flag` | Flag logic (section 2) |
| `finVal` | 1 argument, `Nat` | The value of a `Fin n` (section 2) |
| `finPair`, `finFirst`, `finSecond` | 4, 3 and 3 arguments, `Fin` | A joint value of `Fin (natMul x y)` and its 2 parts. The first part is the low digit. (section 2) |
| `finSub`, `finMax` | 2 arguments, `Fin` | Fin subtraction and maximum (section 2) |
| `finEq`, `finLt`, `finLe` | 2 arguments, `Flag` | Fin comparison (section 2) |
| `allFin` | 2 arguments, `Flag` | A predicate is true on each value of `Fin n` (sections 5 and 7) |
| `ratAdd`, `ratSub`, `ratMul`, `ratDiv` | `Rat -> Rat -> Rat` | Exact fraction arithmetic (sections 2 and 5) |
| `ratOfNat` | `Nat -> Rat` | The fraction of a Nat (sections 2 and 5) |
| `ratEq`, `ratLe`, `ratLt` | `Rat -> Rat -> Flag` | Rat comparison (sections 2 and 5) |
| `sumRat` | 2 arguments, `Rat` | The sum of a function over `Fin n` (sections 5 and 7) |
| `matTabulate`, `matOfFn` | 3 arguments, `Matrix` | A matrix from a function to `Rat`, or the deterministic kernel of a function on `Fin` (section 3) |
| `matEntry` | 3 arguments, `Rat` | One entry of a matrix (section 3) |
| `matComp`, `matKron` | 5 and 6 arguments, `Matrix` | Composition and Kronecker product (section 3) |
| `matEq` | 2 arguments, `Flag` | Matrix equality (section 3) |

An overflow or a division by 0 traps: `EVAL_OVERFLOW`
(`test/emit/trap.lang`) and `EVAL_DIV_ZERO` (`test/emit/divzero.lang`).

The domain files hold 190 definitions: `domain/domain.lang` 5,
`domain/finstoch.lang` 10, `domain/auction.lang` 81 and
`domain/opengame.lang` 94. The Type column gives the place of the
definitions.

| Operation | Type | Meaning (design section) |
|---|---|---|
| `matId`, `matCopy`, `matDiscard`, `matBraiding`, the unitors and their inverses, `matAssociator`, `matAssociatorInv` | See `domain/finstoch.lang` | The structure maps of FinStoch (auction-cat `MarkovCat/FinStoch.lean`) (section 3) |
| `truthful`, `halfShading` | See `domain/auction.lang:8` | Bid strategies `Fin n -> Fin n` (auction-cat `Examples.lean:202`, `Bidder.lean:114`) (section 4) |
| `vickreyUtility`, `fpsbUtility`, `fpsbReserveUtility`, `vickreyReserveUtility` and the forms for bidders 2 and 3 | See `domain/auction.lang:13` and `:36` | The utility of one bidder in the second price, first price and reserve auctions, for 2 and 3 bidders (sections 2 and 4) |
| `spsbFn`, `fpsbFn`, `spsbReserveFn`, `fpsbReserveFn`, `spsb3Fn`, `fpsb3Fn` and their `At` and `Win` helpers | See `domain/auction.lang:63` | The outcome of each bidder over the joint bid: `finPair 2 n allocation price` (section 4) |
| `secondPriceSealedBid`, `firstPriceSealedBid`, `spsbReserve`, `fpsbReserve`, `dutchAuction`, `englishAuction`, the forms with suffix 3, `biddedMechanism` | See `domain/auction.lang` | The mechanisms as matrices (auction-cat `Dutch.lean:42`, `English.lean:41`, `Auction.lean:168`) (section 4) |
| `outcomeRevenue`, `uniformPrior`, `expectedRevenue` and the forms with suffix 3 | See `domain/auction.lang:135` | Outcome revenue, uniform priors and expected revenue under the supplied prior (auction-cat `Revenue.lean:62-241`, `Revenue3.lean:34-65`) (section 5) |
| `vickreyExpectedUtility`, `fpsbExpectedUtility`, `fpsbReserveExpectedUtility` and the forms for bidders 2 and 3 | See `domain/auction.lang:152` | The expected utility of one bidder (auction-cat `BayesNash.lean:56-669`) (section 5) |
| `vickreyAllocation`, `vickreyExpectedPayment`, `vickreyEqUtility`, `vickreyEnvelopeIntegral`, `paymentFromAllocation` | See `domain/auction.lang` | The envelope objects of the Vickrey auction (section 5) |
| `gameId`, `gameComp`, `gameScore` | See `domain/opengame.lang:9` | The game layer (auction-cat `OpenGamesCat/Basic.lean:50`, `:55`, `:68`) (section 6) |
| `middleInterchangeFn`, `matMiddleInterchange`, `gameKron` | See `domain/opengame.lang:16` | The middle interchange (`Basic.lean:80`) and the Kronecker product of two games (section 6) |
| `makeBidder`, `truthfulBidder`, `deviatorBidder`, `halfShadeBidder` | See `domain/opengame.lang:24` | A bidder as an open game (auction-cat `Bidder.lean:49`) (section 6) |
| `auctionGame`, `auctionScore`, `spsbAuction`, `fpsbAuction`, the reserve forms and the deviator forms | See `domain/opengame.lang:34` | The 2-bidder auction games (auction-cat `Auction.lean:38-160`) (section 6) |
| `auctionGame3`, `auctionScore3`, `spsb3Auction`, `fpsb3Auction`, the reserve forms and the deviator forms | See `domain/opengame.lang:72` | The 3-bidder auction games (auction-cat `Auction.lean:201-318`) (section 6) |
| `auctionGame3Det` and `auctionGame3Deviator1Det` to `auctionGame3Deviator3Det` | See `domain/opengame.lang:95` | The factored 3-bidder games: the view and the update are the `matOfFn` of their Fns (auction-cat `Vickrey3.lean`). The 3-bidder scores use them, so they work at n = 3 (O7) (section 6) |
| The `Fn` and `Util` forms, for example `spsbAuctionFn` and `auctionBidder1Util` | See `domain/opengame.lang:137` | The deterministic function of each auction score and game (section 6) |
| `auctionExpectedBidder1Util`, `vickreyReserveExpectedUtility` and the forms for bidders 2 and 3 | See `domain/opengame.lang:191` | The Bayes-Nash pipeline expectations (auction-cat `BayesNashPipeline.lean`) (sections 5 and 6) |

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
  `test/eval/expect.txt`. RULED 2026-10-08 (USER): keep. The 9 bayesnash
  eval rows stay.
- O2. `design/DESIGN.md` did not hold the domain design. RULED 2026-10-08
  (USER): write it. Done: `design/DESIGN.md`, from auction-cat.
- O3. `probe/CAPABILITY.md` was the unfilled template. RULED 2026-10-08
  (USER): write it. Done: `probe/CAPABILITY.md`.
- O4. The matrix limit. RULED 2026-10-08 (USER): the largest limit that
  is free. A matrix has at most 1 << 25 rows or columns, and 1 << 25
  cells (`src/front/eval.c:1100-1101`). A cell is 16 bytes, so a matrix
  at the limit takes 512 MiB. This is half of the 1 GiB arena
  (`src/main.c:10`). 1 << 26 cells take 1 GiB and 16 bytes, so they do
  not fit. The size check runs before the allocation
  (`src/front/eval.c:1127-1131`), so a refusal uses no memory. A larger
  matrix refuses with `EVAL_MATRIX_SIZE` (`test/emit/mat-size.lang`,
  `test/emit/auction3-size.lang`). This limit applies to intermediate
  matrices too. Ordinary 3-bidder mechanisms work at n = 3 and n = 5;
  `test/emit/auction3-size.lang` checks their size refusal at n = 13.
  The kron form of the 3-bidder open game (`auctionGame3`) exceeds the
  limit at n = 3. The 3-bidder scores use the factored games of slice B4
  (O7), so they work at n = 3; `test/emit/opengame-size.lang` checks their
  refusal at n = 4. A higher limit needs a larger arena, and that is
  not free.
- O5. The scope of M1. The core operations of `design/DESIGN.md` section 8
  exist since M0 (section 6). What must M1 add? Options: (a) checks of the
  core operations in 4 slices: B1 tests for the 22 domain definitions that
  no example or test uses; B2 the FinStoch laws at fixed sizes; B3 the
  auction-cat results of O6; B4 `auctionGame3` at n = 3 (O7). (b) B1 and
  B2 only; B3 moves to M2 and B4 moves to M3. (c) No slices: M0 gives the
  operations, so M1 closes now. Proposal: (a). RULED 2026-10-09 (USER):
  "a (B1 to B4)". M1 has the slices B1 to B4, and B5 closes M1.
  DONE 2026-10-09 (slice B5): B1 to B4 are done, and B5 closes M1.
- O6. The auction-cat results that M0 does not compute. These are the
  dominant-strategy truthfulness results
  (`AuctionCat/SecondPrice.lean:149`, `AuctionCat/ReserveTruth.lean:53`,
  `AuctionCat/Reserve3Truth.lean:812`), the 3-bidder Dutch and English
  results (`AuctionCat/Dutch3.lean:38`, `AuctionCat/English3.lean:39`),
  the expected revenue comparison
  (`AuctionCat/ExpectedRevenueComparison.lean:84`) and the 3-bidder first
  price reserve kernel form (`AuctionCat/KernelFirstPrice3.lean:346`).
  M0 already checks the 2-bidder first price kernel form
  (`AuctionCat/KernelFirstPrice.lean:96`) with `fpsbFnEq` at n = 2 and 3
  (`examples/opengame.lang:6`, `test/eval/expect.txt:214-215`). A check of
  the remaining results at fixed sizes uses `allFin`, `ratLe` and
  `matEq`. `design/DESIGN.md` section 7 does not list these results.
  Options: (a) check the remaining results at fixed sizes and add rows for
  all these results to `design/DESIGN.md` section 7; (b) check the
  remaining results, and list them in `docs/VALIDATION.md` only;
  (c) do not check them in M1. Proposal: (a). RULED 2026-10-09 (USER):
  "a (check them at fixed sizes and add DESIGN.md section 7 rows)". Slice
  B3 does this. DONE in B3 (2026-10-09): `examples/truthful.lang` has 11
  checks (`vickTruth3`, `vickTruth5`, `vickResTruth3`, `vickResTruth5`,
  `spsb3ResBidder3Dom3`, `revGe2`, `revGe3`, `revGe4`, `revGe5`,
  `fpsbKernel4` and `fpsb3ResKernel2`). B1 checks the Dutch and English
  results (`examples/auction.lang`: `dutch3Eq3` and `english3Eq3`).
  `design/DESIGN.md` section 7 has a row for each result.
- O7. `auctionGame3` at n = 3. Its kron form exceeds the matrix limit of
  O4, and before slice B4 its score kernels ran at n = 2 only.
  Options: (a) a factored construction: build each 3-bidder kernel as the
  `matOfFn` of a function, so that no intermediate matrix has more than
  1 << 25 cells; (b) keep the limit, and move this item to M3; (c) a larger
  arena (O4: not free). Proposal: (a) if a size count shows that it fits,
  else (b). RULED 2026-10-09 (USER): "a if the size count fits, else b".
  Slice B4 makes the size count first.
  DONE 2026-10-09 (slice B4): the size count fits, so (a). At n = 3 the
  kron form has a 5,832 x 5,832 matrix (34,012,224 cells). The factored
  games (`auctionGame3Det` and the 3 deviator forms in
  `domain/opengame.lang`) build the view and the update as the `matOfFn`
  of their Fns. Their largest matrix is the kron of the identity of size
  27 and the 27 x 216 mechanism: 729 x 5,832 (4,251,528 cells). The score
  kernels run at n = 3. At n = 4 this kron has 134,217,728 cells and
  refuses.
- O8. The place of the new checks. Options: (a) new example files, for
  example `examples/finstoch-laws.lang` and `examples/truthful.lang`
  (`examples/laws.lang` exists already). Each new file adds 1 example and
  1 JSON build to the gate. (b) Add them to `examples/matrix.lang`,
  `examples/dominance.lang` and the other existing files. The example and
  JSON build counts stay at 20. Proposal: (b) for B1, (a) for B2 and B3.
  RULED 2026-10-09 (USER): "b for B1, a for B2 and B3". The B1 checks go
  into the existing example files. DONE 2026-10-09: B1 adds its checks to
  4 existing example files. B2 and B3 add `examples/finstoch-laws.lang`
  and `examples/truthful.lang`.
- O9. The scope of M2. The template row is "Queries and reads, or the
  contract" (section 10). In lang-template, "the contract" is the M2 of a
  contract host: the contract writer of an EVM target. auction-lang has the
  tcc-json host, and its target is one JSON document (sections 7 and 8).
  No program reads that document (`probe/CAPABILITY.md:53`). Options:
  (a) queries and reads: a reader for the JSON document of `langc build`,
  and queries over the instances that it reads (the slices C1 to C4 in
  section 10); (b) the contract: a second target, an EVM contract for one
  mechanism, from a contract host kit of lang-template. This is a new host,
  and sections 7 and 8 do not include it; (c) no M2 slices: M2 closes now,
  and M3 starts. Proposal: (a).
  RULED 2026-10-09 (USER): "a (queries and reads)".
  DONE 2026-10-09 (slice C4): C1 to C3 are done, and C4 closes M2.
- O10. The form of a query (only with O9 a). Options: (a) a verb
  `langc query DOC NAME` that prints the value of one instance of the
  document. The language does not change. (b) The instances of a document
  become definitions of a program: `langc eval PROG NAME --read DOC` and
  `langc build PROG --read DOC`. A query is a definition of the program,
  and the checker checks its type. (c) Both: (a) in C2 and (b) in C3.
  Proposal: (b). Then the tests of a query are programs in this language
  with eval lines, and a query can use each domain operation.
  RULED 2026-10-09 (USER): "b (read instances become definitions)".
  DONE 2026-10-09 (slice C3): the forms are `langc eval PROG --read DOC
  NAME [ARGS...]` and `langc build PROG --read DOC [-o OUT]`. The 2 words
  `--read DOC` come right after `PROG`.
- O11. The gate counts for M2 (only with O9 a). The gate prints 7 counts
  (`test/gate.sh`). Options: (a) 2 new count lines: the round trips of the
  JSON goldens and the read refusals; (b) count the round trips with the
  JSON builds and the read refusals with the build refusals, so that the
  gate keeps 7 count lines; (c) a separate target `make check-read`.
  Proposal: (a).
  RULED 2026-10-09 (USER): "a (2 new count lines)".
  DONE 2026-10-09: slice C1 adds the 2 count lines `round trips` and
  `read refusals`. Slice C2 adds `reads` and `typed reader regressions`,
  and slice C3 adds `queries`. The gate prints 12 counts.
- O12. Two open choices of slice B4. First, the 3-bidder scores keep the
  kron of the identity of size 27 and the mechanism: 729 x 5,832
  (4,251,528 cells) at n = 3 (O7). At n = 4 this kron has 134,217,728
  cells and refuses. Second, the kernels `spsb3Dev1FnEq`, `spsb3Dev2FnEq`,
  `res3FnEq2` and `res3Dev1FnEq2` to `res3Dev3FnEq2` have eval lines at
  n = 2 only (`test/eval/expect.txt:236-237,244-247`). One factored kernel
  at n = 3 took 11.75 s (`docs/VALIDATION.md`, Slice B4). Options:
  (a) move both to M3 (hardening: limits and speed): a mechanism step with
  no kron of an identity, so that the 3-bidder scores run at n = 4, and
  n = 3 eval lines for these kernels if the gate time permits; (b) do both
  in M2, as a slice before C1; (c) close both: the kron form follows the
  score of auction-cat, and the n = 2 lines stay. Proposal: (a).
  RULED 2026-10-09 (USER): "a (D192 and D193 to M3)".

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
| A6 | bd755d4 | This file, `docs/STATUS.md`, `docs/VALIDATION.md`, `README.md`, and 2 line citations in `docs/host/CAPABILITY.md` |
| Post-A6 | ed8da0a | The rulings on O1 to O4, the matrix limit 1 << 25, `probe/CAPABILITY.md` and `design/DESIGN.md` |

M0 is done (2026-10-09). The slices A1 to A6 and the post-A6 commit give
it, and O1 to O4 are ruled.

M1 has these slices. The rulings on O5 to O8 are in section 9
(2026-10-09).

| Slice | Commit | Content |
|---|---|---|
| B1 | efd75c6 | 22 eval lines for the 22 domain definitions that no example or test used |
| B2 | 56ed192 | 15 eval lines in the new example `examples/finstoch-laws.lang`: 12 FinStoch laws at fixed sizes, with `matEq`, and 3 stochastic matrices for them |
| B3 | cea7627 | 11 eval lines in the new example `examples/truthful.lang`: the auction-cat results of O6 at fixed sizes, with `allFin`, `natLe`, `ratLe` and `matEq` |
| B4 | f2d43c9 | `auctionGame3` at n = 3 (O7 a): the factored games in `domain/opengame.lang`, 4 eval lines in `examples/opengame.lang`, and the size refusal at n = 4 |
| B5 | c2a561b | Close M1: this file, `docs/STATUS.md` and `docs/VALIDATION.md`. No test changes |

M1 is done (2026-10-09). The slices B1 to B5 give it, and O5 to O8 are
ruled. The core operations of M1 exist since M0 (section 6), and B1 to B4
add their tests.

M2 has these slices (2026-10-09). The rulings on O9 to O12 (section 9)
keep the plan of the proposals. With O9 (a), M2
adds a reader for the JSON document of `langc build`, and queries over
the instances that it reads.

| Slice | Commit | Content |
|---|---|---|
| C1 | a96d772 | The JSON reader: `langc read DOC` reads a document of format version 1 and writes it again. Each of the 22 JSON goldens reads and writes back with no change. New read refusals for syntax, depth, size, version and shape |
| C2 | 3a1397a | The typed read: the reader parses and checks each `type` field and decodes each `value` against its type (section 7). The read value of each instance of the 22 examples equals its value from `langc eval`. `langc read DOC` prints one line `name : type = value` for each instance, and `langc read --json DOC` keeps the output of C1. New read refusals for names, types and values |
| C3 | 1c3128f | The queries (O10 b): `langc eval PROG --read DOC NAME [ARGS...]` and `langc build PROG --read DOC [-o OUT]`. The 2 words `--read DOC` come right after `PROG`. Each instance of the document becomes a definition of the program, with its read type and value, before the definitions of the program. A query is a definition of the program. With `--read`, each instance name must be one identifier, else `READ_NAME`. An instance name that is a core name, a domain name or a name of the program is `REFUSE_NAME`. `langc build` with `--read` writes the instances of the program only. The query tests are in `test/query/` |
| C4 | This commit | Close M2: this file, `docs/STATUS.md`, `docs/VALIDATION.md`, `probe/CAPABILITY.md` and `README.md`. `docs/host/README.md` has the text of C1 to C3 and does not change. No test changes and no source changes |

M2 is done (2026-10-09). The slices C1 to C4 give it, and O9 to O12 are
ruled. A program can use the instances of a JSON document of
`langc build` (`langc eval PROG --read DOC NAME`).

M3 is not started. With O12 (a), M3 also holds the two open choices of
slice B4.
