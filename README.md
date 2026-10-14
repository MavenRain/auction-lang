# auction-lang

auction-lang is a small checked language for auctions over finite values.
It ports the auction results of auction-cat (Lean 4) to programs. The
compiler checks a program, evaluates it and writes JSON. It also reads
that JSON again, and a program can use the instances of a read document.
`langc verify` checks that a JSON document is the output of its program.

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

Compare a JSON document with the program that builds it. The output is
`domRes2 : Flag = 1`. A document that is not the output of `langc build`
for the program is refused.

```
build/langc verify examples/dominance.lang test/json/dominance.json
```

Use the instances of a document in a program (a query). Each instance
becomes a definition before the definitions of the program. The output is
`4/9`.

```
build/langc eval test/query/matrix.lang --read test/json/matrix.json corner
```

Each line of `test/query/expect.txt` gives one such query and its result.

## Book coverage

Milestone M4 adds nine examples for selected models in Menezes and
Monteiro's [An Introduction to Auction Theory](https://academic.oup.com/book/25449)
(2004 edition, M&M). The examples use finite type and bid spaces, or
continuous payoff formulas checked at finitely many sample points.
Each headline result is a closed `Flag` that evaluates to 1, and key
`Rat` values are goldens in `test/eval/expect.txt`. A passing check
establishes the implemented comparisons for the stated model and
samples. When a finite model differs from the continuous model, the
example describes the difference and checks its result ("as found").
The table gives partial coverage of the book.

Milestone M5 adds proofs. A `theorem` is a proof term over the kernel
rules of `kernel/rules.lang` (`SPEC.md` section 11), and `langc check`
accepts it or refuses it with a code. The files `test/proof/ok/l1.lang`
to `l11.lang` hold 444 theorem checks for the book, and `k1.lang` to
`k4.lang` hold 110 for the kernel rules: 554 in total. M6 session 2
adds `l7.lang` (Ch. 5 and App. C), M6 session 3 adds `l8.lang` (the
explicit 2-bidder allocation of ch. 6.4), M6 session 4 adds `l9.lang`
(no pure symmetric equilibrium in 3.4 for all Rat bids), M6 session 5
adds `l10.lang` (the sequential game form of ch. 7), and M6 session 6
adds `l11.lang` (package auctions of ch. 7). M6 session 7 adds `k4.lang`
(the N-ary profile layer: `natPow`, `finProj`, `finUpd` and 6 kernel
rules). A file cannot import
another file, so the files identify their copied
helpers, and a helper counts once per file. The column `Proved for all sizes (M5)` names the
theorems that hold for every grid size, every value or every prior, with
their file, subject to each theorem's hypotheses. A result at the M4
sizes by `decide` is marked as such.

| Book section | Example | What is checked | Proved for all sizes (M5) |
|---|---|---|---|
| Ch. 2 and 3.1, private values | the 22 examples of M0 to M3, for example `truthful.lang` | the auction-cat results at fixed sizes | `l1.lang`: `fpsb_spsb_same_allocation`, `fpsb_revenue_ge_spsb`, `english_revenue_eq_spsb`, `dutch_revenue_eq_fpsb`, `vickrey_truthful_dominant` (every n), `vickrey_truthful_is_bayes_nash`; `l1b.lang`: `fpsb_revenue_eq_max`, `spsb_revenue_eq_min`, `dutch_revenue_ge_english`, the `fpsbReserve_*`, `spsbReserve_*` and `reserveRevenue_*` theorems; `l6.lang`: `vickrey3_truthful_dominant`, `vickrey3_truthful_best_response` (3 bidders) |
| 3.1.1, the first price bid | `discrete.lang` | for continuous uniform values on [0, 1], the payoff formula for the bid (n - 1) v / n is checked against report samples z = k/8, for n = 2 and n = 3 | `l2.lang`: `fpsb_uniform_bid_best_reply_n2` (all Rat values and reports), `fpsb_uniform_bid_best_reply_n3` (nonnegative values and reports). The payoff polynomial is a definition |
| 3.1.4, entry fees | `entryfee.lang` | with i.i.d. values uniform on {0, 1, 2, 3}, the entry cutoff equilibrium and equal reserve and entry fee revenues; the best cutoff is 2, with fee 1 and revenue 25/16 for 2 bidders, or fee 1/2 and revenue 61/32 for 3 bidders | `l2.lang`: `spsb_reserve_half_optimal_uniform`, `spsb_reserve_half_revenue_uniform`, `spsb_no_reserve_revenue_uniform`, `spsb_reserve_above_support_revenue_uniform`, `spsb_reserve_support_boundary_revenue_uniform` (two uniform bidders, every reserve r at least 0, the revenue R as a definition); `l6.lang`: `entry_fee_closed_form` (for K > 0 and c <= K, the fee at cutoff c on a uniform grid of K values is c * c / K) |
| 3.2, correlated values | `correlated.lang` | second price truthful bids are dominant; first price equilibria on a grid; revenues for an independent and a correlated prior. As found: first price revenue 3/4 is more than second price revenue 8/11 for the correlated prior | `l2.lang`: `vickrey_truthful_best_response_joint` (3.2.1, the conditional belief of any joint prior) |
| 3.3, risk aversion | `riskaverse.lang` | with continuous uniform values and utility sqrt(x), sampled best replies for bids 2v/3 and 4v/5; 2v/3 is more than v/2; the revenue formulas give first price more than second price (4/9 and 1/3 for 2 bidders, 3/5 and 1/2 for 3 bidders) | `l2.lang`: `vickrey_truthful_dominant_utility`, `vickrey_truthful_best_response_utility` (each increasing utility u : Nat -> Rat) |
| 3.4, discrete values | `discrete.lang` | the two-value mixed strategy formula at bid samples (upper support endpoint 5/3, the continuous bid distribution G); first and second price revenue formulas both give 11/9; no symmetric pure equilibrium on the chosen bid grid of 1/8 | `l2.lang`: `ds_high_indifferent`, `ds_high_above_bbar_worse`, `ds_G_at_bbar`, `ds_G_at_vL`, `ds_revenue_equivalence` (all vL, vH and p), `ds_book_revenue`, `ds_book_bbar`. The indifference and G identities require nonzero divisors. `l9.lang`: `no_pure_symmetric_equilibrium` (all vL < vH, 0 < p < 1 and all Rat bids bL and bH), `no_pure_equilibrium_half` (vL = 0, vH = 1 and p = 1/2, as in `discrete.lang`), `l9GridCheck` (by `decide`, the payoff agrees with the grid of `discrete.lang`) |
| Ch. 4, common values | `commonvalue.lang` | for 2 bidders with common value V = s + t and independent uniform [0, 1] signals, sampled best replies for first price b(s) = s and second price b(s) = 2s, with revenue formulas 2/3; on discrete signals, the winner's curse, failure of the naive bid to be an equilibrium, and equilibrium checks for second price 2s. As found: the finite independent-signal model gives first price 13/9 in one of 3 symmetric pure equilibria and second price 10/9 | `l5.lang`: `winners_curse_grid` (every signal grid, every nondecreasing value), `winners_curse_uniform` (all Rat s < 1), `wallet_first_price_best_reply`, `wallet_second_price_best_reply` (the wallet game, all Rat signals), `wallet_revenue_equivalence`; `l6.lang`: `winners_curse_grid_le`, `wallet_naive_bid_loses` (every Rat signal 0 < s < 1); at the M4 sizes by `decide`: `winners_curse_uniform_grid`, `winners_curse_discrete`, `wallet_naive_bid_loses_grid` |
| App. A, probability and order statistics | none (the M4 examples use instances) | none | `l5.lang`: `sumRat_mul_sumRat`, `expect_linear`, `expect_const`, `expect_indep_mul`, `total_expectation`, `max_cdf_two` (any grid), `uniform_max_two_grid`, `uniform_min_two_grid` and their means (two uniform draws on a grid of size k, exact sums); `l6.lang`: `total_expectation_flat`. Conditional expectations require nonzero mass in each row; the grid means require k > 0. Not proved: order statistics for N > 2 |
| Ch. 5 and App. C, affiliated signals | `affiliated.lang` | affiliation on chosen pair and triple priors; prices under second price and English strategy formulas transplanted to discrete signals. As found: the second price formula fails the equilibrium check for both priors with 3 bidders; its revenue exceeds the English price formula. This comparison is not an equilibrium revenue ranking; the English strategies are not checked for equilibrium | `l7.lang` (M6 session 2): the defs `IsAffiliated2` (affiliation of two signals on a finite grid, App. C) and `IsMonoFin`; `cor_prior_affiliated` and `anti_prior_not_affiliated` (the two priors of `affiliated.lang`, by `decide` through the bridge `l7AffiliatedOfFlag`); `independent_prior_affiliated` (every product prior, all sizes); `chebyshev_sum_inequality` (the Harris inequality on a chain: weights at least 0, two monotone functions); `affiliated_cond_expectation_increasing` (for an affiliated prior and a monotone a, E[a(X) given Y = y] is at most E[a(X) given Y = z] when y is at most z); `mlrp_implies_fosd` (the conditional upper tail P(X at least k given Y = y) increases in y). The last two are cross-multiplied, with no division |
| Ch. 6, mechanism design | `mechanism.lang` | incentive compatibility on discrete types, the revelation principle, the payment band, discrete virtual values, the optimal auction (revenue 13/8 = the expected virtual surplus), VCG dominance. As found: revenue equivalence fails (53/64, 7/8 and 17/16 with the same allocation); second price with the optimal reserve 2 gets 25/16, less than 13/8 | `l3.lang`: `revelation_principle` (6.1), `ic_monotone`, `ic_payment_band` (6.2), `ic_ir_payment_upper`, `ic_payment_lower` (6.3), `ic_ir_revenue_le_virtual_surplus` (6.4, every IC and IR mechanism, every prior and type count), `revenue_equivalence`, `revenue_equivalence_via_allocation` (3.1.3, the Vickrey payment identity for all n and priors, and equality of envelope-defined payments for equal allocations); `l4.lang`: `virtual_value_auction_attains_bound` (6.4, every monotone nonnegative Q), `vcg_truthful_dominant` (6.5, assuming an efficient allocation), `vcg_individually_rational` (also assuming pivot payments and nonnegative own valuations); `l8.lang`: `two_bidder_allocation_feasible`, `two_bidder_tie_split` (6.4, the higher nonnegative virtual value wins and a tie splits 1/2), `two_bidder_interim_monotone` (a nondecreasing virtual value gives a nondecreasing interim allocation), `two_bidder_auction_attains_bound` (6.4, every type count and every nonnegative prior whose discrete virtual value is nondecreasing: IC, IR, and the revenue is the expected virtual surplus); `l8InstanceCheck` checks the 4-type instance of `mechanism.lang` by `decide` (revenue 13/8) |
| Ch. 7, identical units | `multiunit.lang` | for 3 unit demand bidders and 2 units, uniform price truthful bids are dominant on the bid grid (revenue 9/8), and 2 pay as bid equilibria are checked; a separate bidder wanting 2 units gains by reducing demand, while multi unit Vickrey reports are dominant on the report grid. As found: pay as bid revenues are 63/64 and 293/256; the sequential sale has no monotone symmetric pure equilibrium on the chosen bid grid; the conditional-expectation bidding rule transplanted to discrete values fails the equilibrium check and gives E[p1] = 62/105 > E[p2] = 9/16 | `l4.lang`: `uniform_price_unit_demand_dominant` (3 bidders, 2 units, all Rat values and bids), `multi_unit_vickrey_dominant` (the VCG instance at Fin 3); at the M4 sizes by `decide`: `demand_reduction`. `l10.lang`: `sequential_round2_truthful_dominant` (2 rounds of second price and a round 1 winner leaves: a truthful round 2 bid is dominant for every history and all Rat values, round 1 prices and bids), `sequential_nonmonotone_not_equilibrium` (by `decide` on the bids k/4: a round 1 profile that is not monotone in the value is not an equilibrium) |
| Ch. 7, two objects | `combinatorial.lang` | VCG truthful reports are dominant and individually rational; low revenue and an outcome that is not in the core; revenue that is not monotone in the bidders; expected revenue 25/32; the exposure problem of a synergy bidder in two separate second price auctions | `l4.lang`: `two_object_vcg_dominant` (the VCG instance at Fin 4, all values, assuming an efficient allocation), `two_object_vcg_ir` (also assuming pivot payments and nonnegative own valuations); at the M4 sizes by `decide`: `revenue_not_monotone`, `vcg_outcome_not_in_core`, `exposure_problem`. `l11.lang`: `package_wd_maximizes_welfare` (package bids on A, B and AB, pay as bid: the winner determination maximizes the reported welfare for all Rat bids); by `decide` on the grid: `package_wd_agrees_vcg_allocation` (the allocation agrees with VCG), `package_truthful_not_equilibrium` (truthful package bids are not an equilibrium), `package_revenue_vs_vcg` (at truthful bids the revenue is at least the VCG revenue; expected 207/64 against 25/32) |

Not covered after M5:

- Appendix B, the differential equations of the equilibrium bids. The
  examples check closed forms at sample points, or discrete analogues.
- The exercises.
- Every result for a general continuous distribution F (densities and
  integrals): the derivation of the first price bid formula, revenue
  equivalence and the optimal auction with densities, the linkage
  ranking (5.5), risk aversion with a general F (3.3.1), the correlated
  comparison (3.2.3), affiliated first price and risk aversion (5.3 and
  5.6) and the martingale property (7.2). A continuous payoff or
  revenue formula enters a theorem as a definition.
- In App. C, the Harris inequality for 2 variables (a product prior on a
  2-D grid), and App. D. In ch. 5, prices under strategies that fail an
  equilibrium check do not establish an equilibrium revenue ranking or a
  failure of the martingale property at equilibrium.
- In ch. 3, no pure symmetric equilibrium (3.4) for more than 2 values
  (bid functions Fin k -> Rat). `l9.lang` proves it for 2 values and all
  Rat bids.
- In ch. 6, ironing (a prior whose discrete virtual value is not
  nondecreasing: `two_bidder_auction_attains_bound` in `l8.lang` takes
  regularity as a hypothesis), and the correlated and common value
  extensions (sections 6.4.2 and 6.4.4).
- In ch. 7, the optimal multiple-object auction.
- Mechanism matrices for every number of bidders: auction-cat has 2 and
  3 bidder mechanisms. Order statistics for N > 2.
- The continuous results whose assumptions change in examples marked
  "as found".

## Files

- `SPEC.md`: the specification.
- `docs/STATUS.md`: what works, the remaining work and the known limits.
- `docs/VALIDATION.md`: the gate results for each slice.
- `docs/host/README.md`: the commands, the JSON encoding and the build of
  the host kit.
- `docs/host/CAPABILITY.md`: the host facts and limits, each with its test.
- `domain/`: the domain files. Each name in them is a core name.
- `examples/`: 31 example programs.
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
