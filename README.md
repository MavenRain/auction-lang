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

| Book section | Example | What is checked |
|---|---|---|
| Ch. 2 and 3.1, private values | the 22 examples of M0 to M3, for example `truthful.lang` | the auction-cat results at fixed sizes |
| 3.1.1, the first price bid | `discrete.lang` | for continuous uniform values on [0, 1], the payoff formula for the bid (n - 1) v / n is checked against report samples z = k/8, for n = 2 and n = 3 |
| 3.1.4, entry fees | `entryfee.lang` | with i.i.d. values uniform on {0, 1, 2, 3}, the entry cutoff equilibrium and equal reserve and entry fee revenues; the best cutoff is 2, with fee 1 and revenue 25/16 for 2 bidders, or fee 1/2 and revenue 61/32 for 3 bidders |
| 3.2, correlated values | `correlated.lang` | second price truthful bids are dominant; first price equilibria on a grid; revenues for an independent and a correlated prior. As found: first price revenue 3/4 is more than second price revenue 8/11 for the correlated prior |
| 3.3, risk aversion | `riskaverse.lang` | with continuous uniform values and utility sqrt(x), sampled best replies for bids 2v/3 and 4v/5; 2v/3 is more than v/2; the revenue formulas give first price more than second price (4/9 and 1/3 for 2 bidders, 3/5 and 1/2 for 3 bidders) |
| 3.4, discrete values | `discrete.lang` | the two-value mixed strategy formula at bid samples (upper support endpoint 5/3, the continuous bid distribution G); first and second price revenue formulas both give 11/9; no symmetric pure equilibrium on the chosen bid grid of 1/8 |
| Ch. 4, common values | `commonvalue.lang` | for 2 bidders with common value V = s + t and independent uniform [0, 1] signals, sampled best replies for first price b(s) = s and second price b(s) = 2s, with revenue formulas 2/3; on discrete signals, the winner's curse, failure of the naive bid to be an equilibrium, and equilibrium checks for second price 2s. As found: the finite independent-signal model gives first price 13/9 in one of 3 symmetric pure equilibria and second price 10/9 |
| Ch. 5 and App. C, affiliated signals | `affiliated.lang` | affiliation on chosen pair and triple priors; prices under second price and English strategy formulas transplanted to discrete signals. As found: the second price formula fails the equilibrium check for both priors with 3 bidders; its revenue exceeds the English price formula. This comparison is not an equilibrium revenue ranking; the English strategies are not checked for equilibrium |
| Ch. 6, mechanism design | `mechanism.lang` | incentive compatibility on discrete types, the revelation principle, the payment band, discrete virtual values, the optimal auction (revenue 13/8 = the expected virtual surplus), VCG dominance. As found: revenue equivalence fails (53/64, 7/8 and 17/16 with the same allocation); second price with the optimal reserve 2 gets 25/16, less than 13/8 |
| Ch. 7, identical units | `multiunit.lang` | for 3 unit demand bidders and 2 units, uniform price truthful bids are dominant on the bid grid (revenue 9/8), and 2 pay as bid equilibria are checked; a separate bidder wanting 2 units gains by reducing demand, while multi unit Vickrey reports are dominant on the report grid. As found: pay as bid revenues are 63/64 and 293/256; the sequential sale has no monotone symmetric pure equilibrium on the chosen bid grid; the conditional-expectation bidding rule transplanted to discrete values fails the equilibrium check and gives E[p1] = 62/105 > E[p2] = 9/16 |
| Ch. 7, two objects | `combinatorial.lang` | VCG truthful reports are dominant and individually rational; low revenue and an outcome that is not in the core; revenue that is not monotone in the bidders; expected revenue 25/32; the exposure problem of a synergy bidder in two separate second price auctions |

M4 does not cover:

- Appendix B, the differential equations of the equilibrium bids. The
  examples check closed forms at sample points, or discrete analogues.
- The exercises.
- General results for all n or all distributions. The checks use 2 or 3
  bidders and grids of at most 13 points.
- In ch. 5, first price equilibrium bidding and the effects of risk
  aversion with affiliated signals (sections 5.3 and 5.6).
- In ch. 6, the bound "each incentive compatible and individually
  rational mechanism gets at most the expected virtual surplus" for all
  mechanisms, ironing, and the correlated and common value extensions
  (sections 6.4.2 and 6.4.4).
- In ch. 7, sequential profiles that are not monotone, and package
  auctions other than VCG, or the optimal multiple-object auction.
- The continuous results whose assumptions change in examples marked
  "as found". Prices under strategies that fail an equilibrium check
  do not establish an equilibrium revenue ranking or a failure of the
  martingale property at equilibrium.

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
