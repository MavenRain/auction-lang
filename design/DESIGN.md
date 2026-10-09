# auction-lang domain design

This file is the domain design of auction-lang. The USER ruled on
2026-10-08 to write it (`SPEC.md` open item O2). Its source is auction-cat
(Lean 4, read only). It is a design record: `SPEC.md` gives the syntax, the
types, the operations and the rules, and this file gives the model behind
them.

This file is the only source of the core types and the core operations of
auction-lang. `SPEC.md` sections 5 and 6 list them, and each meaning comes
from a section of this file. Do not add a core type or a core operation
that this file does not give.

When the design changes, write the new version here. Then record the change
as an open item in `SPEC.md` section 9.

---

## 1. Stance

The model comes first, and auction-cat gives it. An auction is a Markov
kernel in FinStoch, the category of finite stochastic matrices
(`MarkovCat/FinStoch.lean:433`). A value of the language is a
representation of that model at fixed sizes. auction-cat proves its
results for all sizes. auction-lang computes the same objects at the sizes
that a program gives, and the gate compares each value
(`test/eval/expect.txt`). Thus a proof stays in auction-cat, and a
computed instance is in auction-lang.

## 2. Semantic domain

- A size, a count or an index is a `Nat`. A decision is a `Flag`.
- A valuation, a bid, a bidder or a part of an outcome is a `Fin n`. A
  joint value of two parts is one `Fin (natMul x y)`, with the first part
  as the low digit (`Fin.pair`, `MarkovCat/FinStoch.lean:46`).
- A probability, a utility or a revenue is an exact `Rat`. auction-cat
  uses `Rat` too, so a computed value is the Lean value, with no rounding.
- A kernel from m states to n states is a `Matrix m n`. Each row is a
  probability distribution (`StochasticMatrix`,
  `MarkovCat/FinStoch.lean:433-437`).
- An open game is `OpenGame x s y r m`: a view kernel and an update kernel
  with a middle object m (`OpenGamesCat/Basic.lean:40`).

`SPEC.md` section 5 gives each type and its definition.

## 3. FinStoch structure

The structure of FinStoch gives the matrix operations.

- Composition is `matComp` (`comp`, `MarkovCat/FinStoch.lean:596`). The
  tensor is `matKron` (`kron`, `:483`).
- The identity, the unitors and their inverses, the associator and its
  inverse and the braiding are `matId`, `matLeftUnitor`,
  `matRightUnitor`, `matLeftUnitorInv`, `matRightUnitorInv`,
  `matAssociator`, `matAssociatorInv` and `matBraiding` (`:470`,
  `:770-803`, `:1066-1072`, `:1415`).
- Copy and discard are `matCopy` and `matDiscard` (`:1606`, `:1616`).
- The deterministic kernel of a function is `matOfFn` (`detMatrix`,
  `:1271`). Each structure map in `domain/finstoch.lang` is a `matOfFn`
  (`docs/host/CAPABILITY.md:63-66`).

auction-cat proves the category laws and the coherence laws for all sizes
(`assoc` `:687`, `pentagon_FinStoch` `:1354`, `hexagon_FinStoch` `:1533`).
auction-lang does not prove them. It computes the maps.

## 4. Bidders and mechanisms

- A bid strategy is a function `Fin n -> Fin n`. `truthful` is the
  identity, and `halfShading` shades the bid
  (`AuctionCat/Bidder.lean:114`).
- A mechanism is a kernel from the joint bid to the joint outcome. For
  each bidder, an outcome is a win index (`Fin 2`) and a price (`Fin n`)
  (`docs/host/CAPABILITY.md:75-77`).
- An outcome function gives the outcome of each joint bid, for example
  `spsbFn` (`AuctionCat/SecondPrice.lean:38`). `SPEC.md` section 6 gives
  the outcome functions and their helpers.
- The mechanisms are the second price and the first price sealed bid
  auctions (`AuctionCat/SecondPrice.lean:52`,
  `AuctionCat/FirstPrice.lean:52`), their reserve forms
  (`AuctionCat/Reserve.lean:48`, `:77`) and the forms for 3 bidders
  (`AuctionCat/SecondPrice3.lean:65`, `AuctionCat/FirstPrice3.lean:58`,
  `AuctionCat/Reserve.lean:114`, `:141`).
- auction-cat proves that the Dutch auction is the first price auction
  and that the English auction is the second price auction
  (`AuctionCat/Dutch.lean:48`, `AuctionCat/English.lean:48`).
  auction-lang has `dutchAuction` and `englishAuction` too
  (`docs/host/CAPABILITY.md:72`).

## 5. Priors and expectations

- A prior is a function from a valuation to a `Rat`. `uniformPrior` gives
  each value the same weight (`AuctionCat/Revenue.lean:240`). A program
  can give another prior as a lambda (`docs/host/CAPABILITY.md:80-81`).
- The revenue of an outcome and the expected revenue under a prior are
  `outcomeRevenue` and `expectedRevenue` (`AuctionCat/Revenue.lean:62`,
  `:72`). The forms for 3 bidders are at `AuctionCat/Revenue3.lean:34`
  and `:46`.
- The expected utility of one bidder is `vickreyExpectedUtility`
  (`AuctionCat/BayesNash.lean:56`), with its first price and reserve
  forms.
- The envelope objects of the Vickrey auction are the allocation, the
  expected payment, the utility at equilibrium and the envelope integral
  (`AuctionCat/Envelope.lean:53-76`), with `paymentFromAllocation`
  (`:379`).
- auction-cat adds with `Fin.sumRat` (`MarkovCat/FinStoch.lean:122`). The
  core operation `sumRat` is the same finite sum
  (`docs/host/CAPABILITY.md:46-48`).

## 6. Open games

- An open game has a view `Matrix x (natMul m y)` and an update
  `Matrix (natMul m r) s` (`domain/opengame.lang:8`). The middle object m
  is part of the type (`test/check/opengame-middle.lang`).
- The combinators are the identity, the composition, the score, the
  middle interchange and the Kronecker product
  (`OpenGamesCat/Basic.lean:50`, `:55`, `:68`, `:80`, `:104`). The type of
  each combinator gives the m of its result
  (`docs/host/CAPABILITY.md:87-88`).
- A bidder is an open game (`AuctionCat/Bidder.lean:49`): the truthful
  bidder, a deviator and a half shading bidder (`:84`, `:97`, `:119`).
- The auction games for 2 bidders and for 3 bidders, with a deviator and
  with a reserve, are open games too (`AuctionCat/Auction.lean:38-160`,
  `:201-318`).
- auction-cat proves that the middle interchange and the auction kernel
  are deterministic matrices (`AuctionCat/KernelTruth.lean:166`, `:373`).
  auction-lang writes each such kernel as the `matOfFn` of a function, for
  example `spsbAuction n` (`docs/host/CAPABILITY.md:92-93`).

## 7. Results

auction-cat proves most results for all sizes. The auction examples in
auction-lang state the listed results at fixed sizes, and the gate checks
their values. They port the decided forms in `AuctionCat/Examples.lean`.

| Result | auction-cat | auction-lang |
|---|---|---|
| Expected utility of truthful second price versus truthful first price bidding | `auctionExpectedBidder1Util_spsbAuction_ge_fpsbAuction`, `AuctionCat/BayesNashPipeline.lean:1847`; the concrete forms in `AuctionCat/Examples.lean:342-393` | `examples/bayesnash.lang` |
| The same expected utility comparison with a reserve, bidder 2 | `auctionExpectedBidder2Util_spsbReserveAuction_ge_fpsbReserveAuction`, `AuctionCat/BayesNashPipeline.lean:2265` | `examples/dominance.lang` |
| Revenue equivalence | `revenue_equivalence`, `AuctionCat/Envelope.lean:392`, all n | `examples/revenue.lang` |
| The reserve price results | `reserveRevenue_main`, `AuctionCat/Revenue.lean:446`; `optimalReserveExamples_main`, `AuctionCat/Examples.lean:861` | `examples/revenue.lang` |
| The expected utility of one bidder | `vickreyExpectedUtility_truthful_eq_one_uniform`, `AuctionCat/Examples.lean:202`, n = 3 | `examples/utility.lang` |
| The open game kernels | `AuctionCat/KernelTruth.lean`, `AuctionCat/Vickrey3.lean` | `examples/opengame.lang` |
| The Bayes-Nash pipeline expectations | `AuctionCat/BayesNashPipeline.lean` | `examples/bayesnash.lang` |
| Dominant-strategy truthfulness of the second price auction | `vickrey_truthful_dominant`, `AuctionCat/SecondPrice.lean:149`, all n | `examples/truthful.lang`: `vickTruth3`, `vickTruth5` |
| Dominant-strategy truthfulness of the second price auction with a reserve | `vickreyReserve_truthful_dominant`, `AuctionCat/ReserveTruth.lean:53`, all n | `examples/truthful.lang`: `vickResTruth3`, `vickResTruth5` |
| The kernel dominance of bidder 3 in the 3-bidder second price auction with a reserve | `spsb3Reserve_bidder3_kernel_dominance`, `AuctionCat/Reserve3Truth.lean:812`, all n | `examples/truthful.lang`: `spsb3ResBidder3Dom3` |
| The expected revenue of first price is at least the expected revenue of second price for nonnegative prior weights | `expectedRevenue_fpsb_ge_spsb`, `AuctionCat/ExpectedRevenueComparison.lean:84`, all n | `examples/truthful.lang`: `revGe2`, `revGe3`, `revGe4`, `revGe5` |
| The 2-bidder first price kernel form | `fpsbAuction_eq_detMatrix`, `AuctionCat/KernelFirstPrice.lean:96`, all n | `examples/truthful.lang`: `fpsbKernel4`; `examples/opengame.lang`: `fpsbFnEq` |
| The 3-bidder first price kernel form with a reserve | `fpsb3ReserveAuction_eq_detMatrix`, `AuctionCat/KernelFirstPrice3.lean:346`, all n | `examples/truthful.lang`: `fpsb3ResKernel2` |
| The 3-bidder Dutch and English auctions | `dutch3_eq_firstPrice3`, `AuctionCat/Dutch3.lean:38`; `english3_eq_secondPrice3`, `AuctionCat/English3.lean:39`; all n | `examples/auction.lang`: `dutch3Eq3`, `english3Eq3` |

The first two rows compare auctions with truthful bids at the valuations
and priors written in the example files. They do not check bids that
deviate from the valuation. Dominant-strategy truthfulness is a separate
Lean result (`vickrey_truthful_dominant`,
`AuctionCat/SecondPrice.lean:149`, and
`vickreyReserve_truthful_dominant`, `AuctionCat/ReserveTruth.lean:53`).
`examples/truthful.lang` checks these results at fixed sizes (the rows
above).

A program can state and prove an equality over variable sizes and values
when both sides are definitionally equal, or when a supplied equality
proof connects them. For example, this checks for every n and v:

```text
def truthfulIdentity : (n : Nat) -> (v : Fin n) -> Eq (Fin n) (truthful n v) v :=
  fun (n : Nat) => fun (v : Fin n) => refl
```

The kernel has no Sigma eta, so that particular equality fails for a
variable (`probe/CAPABILITY.md` section 8). A program cannot recurse by
name (section 3 of the probe). The auction examples above use computed
values at fixed sizes rather than general equality proofs.

## 8. Admitted operations

Each core operation comes from the model above.

- The Nat, Flag, Fin and Rat operations give the sizes, the indices and
  the exact weights (sections 2 and 5).
- `matTabulate`, `matOfFn`, `matEntry`, `matComp`, `matKron` and `matEq`
  give the kernels and their structure (section 3).
- `allFin` and `sumRat` give the finite quantifier and the finite sum
  (sections 5 and 7).
- The domain definitions give the strategies, the mechanisms, the
  expectations and the open games (sections 4 to 6).

What this refuses: a recursion by name, an axiom and a new family in a
program (`SPEC.md` section 2, rules R1 to R3), and the names of the allow
list in `SPEC.md` section 2, for example `matMul` and `matTranspose`. A
matrix is not a carrier of a structure (`SPEC.md` section 4). The sample
domain of lang-template (`Color`, `Item`, `Stack`, `Lot`) is not part of
this design (`SPEC.md` section 5).
