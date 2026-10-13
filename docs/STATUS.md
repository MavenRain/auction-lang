# auction-lang status

The build implements milestone M0 in the slices A1 to A6 and the post-A6
commit. It also has the core operations of the design (milestone M1 in
`SPEC.md`), and the slices B1 to B4 add their tests. Milestone M2 (the
slices C1 to C4) adds the JSON reader, the typed read and the queries over
the instances of a read document. Milestone M3 (the slices D1 to D6)
adds speed, limits and a checked certificate for the output. M0, M1, M2
and M3 are done. After M3, slice E1 sets the depth limit of a value line
(O18). `SPEC.md` section 9 records the rulings on O1 to O18. Milestone
M4 (book coverage) adds 9 examples. They check finite models and sampled
continuous payoff formulas for selected results of Menezes and Monteiro,
chapters 3 to 7. Book coverage remains partial.

## Implemented

- The type formers F1 to F15 on the tcc-json host (`formers/tcc-json.md`).
- The built-in types `Nat`, `Flag`, `Fin n`, `Rat` and `Matrix m n`, and
  their operations (`src/front/check.c:99-175`).
- `matComp` skips a term when its left cell is 0 (slice D1,
  `src/front/eval.c:1418`). No value and no trap changes.
- The core operation `matIdKronComp` (slice D2): the composition with the
  kron of an identity, with no kron. `gameScore` uses it.
- The kernels of O12 have eval lines at n = 3 (slice D5), and the 3-bidder
  scores have eval lines at n = 4 (slice D2), in `test/eval/expect.txt`.
- `domain/finstoch.lang`: 10 definitions. The structure maps of FinStoch.
- `domain/auction.lang`: 81 definitions. Bid strategies, utilities,
  mechanisms, expected revenue, expected utility and the envelope objects.
- `domain/opengame.lang`: 94 definitions. The `OpenGame` family, bidders,
  the 2-bidder and 3-bidder auction games, their functions and the
  Bayes-Nash pipeline expectations.
- `domain/domain.lang`: 5 definitions and 4 families. The sample domain of
  lang-template.
- 31 example programs in `examples/`. The 9 examples of M4 check selected
  models and formulas (`README.md`, section "Book coverage").
- The verbs `check`, `eval`, `build`, `read` and `verify` of `build/langc`.
  `build` writes one JSON document. `read` reads that document and checks
  each value against its type. With `--read DOC`, `eval` and `build` make
  each instance of the document a definition of the program
  (`docs/host/README.md`).
- The verb `verify PROG DOC` (slice D3): a checked certificate for the
  output. It compares each instance of DOC with the value of that instance
  in PROG, and the bytes of DOC with the bytes that `build PROG` writes
  (`src/main.c:165`, `src/read.c:881`). A difference is a refusal with
  exit 1: `VERIFY_MISSING`, `VERIFY_VALUE`, `VERIFY_EXTRA` or
  `VERIFY_BYTES`.
- Full lines (slice D4): `read` and `eval` print each line in full.
  `value_print_len` and `value_text` (`src/front/eval.c:2247,2251`) count
  the text of a value, then print it into an arena buffer of that size.
  The writer and the reader refuse a JSON `type` longer than 4096 bytes
  with exit 1: `JSON_TYPE_SIZE` (`src/json.c:385-387`) and
  `READ_TYPE_SIZE` (`src/read.c:785`).
- Value depth (slice E1, O18): `read`, `eval` and `verify` print a value
  line of up to 1999 levels in full. A line of 2000 levels or more is
  refused with exit 1: `EVAL_PRINT_DEPTH` in `eval`, `READ_PRINT_DEPTH`
  in `read` and `verify` (`src/front/eval.c:10,2257-2260`). A type and a
  diagnostic keep the cut at 200 levels.
- The gate `make check` (`docs/VALIDATION.md`).

## Remaining work

No M0, M1, M2 or M3 work remains. M4's nine example slices are implemented;
`README.md`, section "Book coverage", lists the remaining book coverage.
`SPEC.md` section 9 records the rulings on O1 to O18, and each ruling is
done. O13 to O17 are the items of M3
(hardening: speed, limits, a checked certificate for the output), and the
rulings of 2026-10-09 accept each proposal. `SPEC.md` section 10 lists
the slices of M0 to M3. Slice D1 is done: `matComp` skips a term when
its left cell is 0, so the 3 slowest eval lines take 3.2 s in place of
19.3 s. Slice D2 is
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
line takes 0.09 s alone. Slice D6 closes M3 and changes docs only.

Slice E1 is done (`SPEC.md` O18, ruled 2026-10-10: the proposal, a limit
of 2000 levels and a refusal past it). Before E1, a value line of
`read`, `eval` or `verify` that nested 200 levels in arguments that are
not the last printed a "..." cut with exit 0. Now it prints in full up
to 1999 levels, and a deeper line is refused (Known limits).

M4 has nine implemented example slices (2026-10-11, ruling: USER).
The examples `entryfee`,
`discrete`, `riskaverse`, `correlated`, `commonvalue`, `affiliated`,
`mechanism`, `multiunit` and `combinatorial` check finite auction models
and continuous payoff formulas at sample points. `README.md` lists the
omitted book results and the scope of each comparison. The largest
example is `examples/multiunit.lang`: `langc build` takes 1.1 s and
895 MB maximum RSS (the arena is 1 GiB).

M5 slice K1 is done (2026-10-11): Prop, theorem, the kernel rules in
`kernel/rules.lang` (True, False, And, Or, Exists) and the kernel
definitions Not, Iff and Ne (`SPEC.md` section 6, Proofs). The gate has
1268 tests, with 2 proof files and 25 proof refusals. The gate also
builds each proof file to JSON and verifies it. 30 K1 mutants are
KILLED.

The staged K1 review also checks that compound proof definitions are
omitted from JSON instances and that a nested core application keeps its
own refusal code. The typed reader regression covers build, read and
verify with proofs of And, Or and Exists alongside a Nat instance.

M5 slice K2 is done (2026-10-11): the kernel definitions Holds, NatLe,
NatLt, RatLe and RatLt, and 37 rules in `kernel/rules.lang`: `decide`,
`natInd`, `flagCases`, the flagIf equations, the order rules of Nat and
Rat, the ratOfNat cast rules and the specifications of the Nat, Flag,
allFin and Fin built-ins (`SPEC.md` section 6, Proofs). The checker
accepts `decide b` only for a closed Flag b that evaluates to flagYes. A
trap is equal to no term, so a type that traps is refused at check with
`TYPE_MISMATCH` (USER ruling, 2026-10-11). Thus 2 build refusals
(auction-overflow, opengame-overflow) are now check refusals. The gate
has 1311 tests, with 3 proof files and 67 proof refusals. 27 of
27 K2 mutants are KILLED.

M5 slice K3a is done (2026-10-12): the deferred K2 rules. Two new
built-ins, `finZero` and `finSucc`, and the kernel definition `natMin`
(USER ruling 6: Nat min is in for M5). `natMin` leaves the refused
names, so the allow-list has 24 names. 13 new rules in
`kernel/rules.lang`: `finCases`, `finEmpty`, `finZeroVal`, `finSuccVal`,
`pairFirst`, `pairSecond`, `pairVal`, `sumRatZero`, `sumRatSucc`,
`foldZero`, `foldSucc`, `minL` and `minR`. The file `test/proof/ok/k3.lang`
has 16 theorems. There are 17 new proof refusals. The gate has 1328
tests, with 4 proof files and 84 proof refusals. 17 of 17 K3a mutants
are KILLED. Next is K3: `nf.c` with `ring`, `ringNat`, `field`, `cert`,
`certLt` and `sqNonneg`.

M5 slice K3b is done (2026-10-12): the checked rules. The new file
`src/front/nf.c` has a normal form of sparse polynomials with Rat
coefficients. There are six new rules in `kernel/rules.lang`: `ring`,
`ringNat`, `field`, `cert` and `certLt` (a check in C), and `sqNonneg`
(a table rule). SPEC.md section 11 has the draft. The file
`test/proof/ok/k3b.lang` has 14 theorems, among them `sumRat_add`,
`sumRat_const_mul` and `sumRat_le`. There are 10 new proof refusals. The
gate has 1339 checks in its main groups, with 5 proof files and 94 proof
refusals, plus 21 C typed reader and 9 C certificate regressions.
15 of 15 K3b mutants are KILLED.

M5 slice L1 is done (2026-10-12): the sumRat library and the first
Revenue results, as proof terms. There are no new kernel rules and no C
changes. The new file `test/proof/ok/l1.lang` has 26 theorems and 3
defs: 7 sumRat theorems (FinStoch.lean:139-245, among them
`sumRat_swap`), 7 helpers with the prefix `l1`,
`fpsb_spsb_same_allocation`, `fpsb_revenue_ge_spsb`, the English and
Dutch revenue equalities (by `refl`), `IsRevenueEquivalent`,
`vickrey_truthful_dominant` for all n, and the Bayes-Nash defs with
`vickrey_truthful_is_bayes_nash`. `sumRat_succ_right`, `sumRat_split`
and `sumRat_unpair` need a Fin cast and are not in L1. The gate has
1340 checks in its main groups, with 6 proof files and 94 proof
refusals.

M5 slice L1b is done (2026-10-12): max, min and reserve revenue, as
proof terms. There is one new kernel rule, `natMaxLub` (the least
upper bound of `natMax`, USER ruling R2 (a)), with the refusal
`l1b-natMaxLub.lang` and 3 of 3 mutants KILLED. The code table in
`src/front/check.c` has the new pair. The new file
`test/proof/ok/l1b.lang` has 49 theorems and no defs: 8 copies from
l1.lang, 18 helpers with the prefix `l1b` (the natMax and natMin
lattice, order and subtraction facts, the closed forms of the four
revenue functions), the 20 Revenue.lean:137-496 theorems from
`fpsb_revenue_eq_max` to `reserveRevenue_eq_zero_of_below_reserve_main`
(the Lean `dutch_revenue_ge_english` is stated on the sealed-bid
functions), `vickrey_bidder2_truthful_dominant` (SecondPrice.lean:127),
and the Nat-indexed `sumRat_succ_right`, `sumRat_split` and
`sumRat_unpair` (FinStoch.lean:188-247, USER ruling R1 (b): the
summand is `g (finVal i)` for `g : Nat -> Rat`). Nothing is cut. The
gate has 1342 checks in its main groups, with 7 proof files and 95
proof refusals.

M5 slice L2 is done (2026-10-12): the Ch 3 ports, as proof terms in
test/proof/ok/l2.lang (33 theorems, 13 defs; 16 theorems and 2 defs are
copies from l1.lang, 9 defs are copies from examples/discrete.lang, the
M4 models). 3.1.1: the first price payoff polynomials are maximized by
reporting the true value for N = 2 and N = 3 (`cert` with `sqNonneg`).
The N = 2 bound holds for all Rat values and reports; N = 3 requires
both to be nonnegative. The uniform auction interpretation uses values
and reports in [0, 1]. 3.1.4: with two uniform bidders the reserve 1/2
gives the most revenue for every r at least 0. The revenue is
2 r^2 (1 - r) + (1 - r)^2 (r + (1 - r)/3) on [0, 1] and 0 above 1;
R(1/2) = 5/12, R(0) = 1/3. Proof regressions check R(1) = R(2) = 0.
3.2.1: the truthful second price bid is a best response under the
conditional belief of any joint prior. 3.3: the truthful second price
bid stays dominant, pointwise and in expectation, under each increasing
utility u : Nat -> Rat. 3.4: the high type is indifferent on
(vL, bbar] (`field`), a bid above bbar is worse (`certLt`), G is 0 at
vL and 1 at bbar, and the first price and second price revenues are
equal for all vL, vH and p (`ring`), with the book case 11/9 and bbar
5/3. The indifference branch and G endpoints require the corresponding
divisors to be nonzero. No kernel change, no new refusal, no mutants.
Not in this slice
(overflow turn): the no pure symmetric equilibrium result for all Rat
bids (3.4) and the 3-bidder second price dominance (Vickrey3.lean:99).
Current state: 8 proof files, 95 proof refusals, 1343 gate tests (the
14 gate rows).

M5 slice L3 is done (2026-10-12): the Ch 6 part 1 ports and the
envelope form of 3.1.3, as proof terms in test/proof/ok/l3.lang (51
theorems, 16 defs; 14 theorems are copies from l1.lang and 2 from
l1b.lang). The Ch 6 mechanism is Nat-indexed: types 0 to n - 1 on the
unit grid, interim allocation Q and payment P : Nat -> Rat, IsICNat and
IsIRNat as Prop defs (examples/mechanism.lang holds the 4-type grid
checks). 6.1: a best reply to a message mechanism gives an incentive
compatible direct mechanism (revelation_principle). 6.2: IC gives
Q (k + 1) >= Q k (adjacent and gap form) and the payment band
k (Q (k + 1) - Q k) <= P (k + 1) - P k <= (k + 1) (Q (k + 1) - Q k)
(`cert` after a `transport` along ofNatAdd). 6.3: with IR the payment
is at most the envelope payment k Q k - sum_{j < k} Q j, and with IC
alone at least that minus Q k - Q 0 - P 0 (natInd with
sumRat_succ_right). 6.4: the expected revenue sum_k f k P k is at most
sum_k f k (envelope payment k), which equals the virtual surplus
sum_j Q j (j f j - sum_{k > j} f k) by a swap of sums (Abel summation);
f phi is in the product form, no division by f. 3.1.3: vickrey_envelope
(Envelope.lean:226), vickrey_utility_plus_payment (:305),
vickrey_payment_formula (:350), revenue_equivalence (:392) and
revenue_equivalence_via_allocation (:400) for all n and all priors, by
a Nat-indexed envelope identity (natInd) and the split of sumRat n at
k (sumRat_split from l1b). No kernel change, no new refusal, no
mutants. Not in this slice (overflow turn): the no pure symmetric
equilibrium result for all Rat bids (3.4), the 3-bidder second price
dominance (Vickrey3.lean:99) and the 3.1.4 entry fee closed form.

M5 slice L4 is done (2026-10-12): the Ch 6 part 2 and Ch 7 ports, as
proof terms in test/proof/ok/l4.lang (55 theorems, 60 defs; 14
theorems are copies from l1.lang, 2 from l1b.lang and 18 from l3.lang;
the examples/multiunit.lang and examples/combinatorial.lang defs are
copies by line number). 6.4: virtual_value_auction_attains_bound,
every monotone nonnegative interim allocation Q (IsMonoNat) with the
envelope payment l3Env Q is IC and IR and its expected revenue is the
virtual surplus l3VS n f Q, the bound of
ic_ir_revenue_le_virtual_surplus in l3.lang; the IC proof bounds the
gap sum between the true type and the report by natInd (l4BelowGe,
l4BelowLe). 6.5: VCG with outcomes Fin m and 2 bidders,
vcg_truthful_dominant for every efficient rule and
vcg_individually_rational for an attained Clarke pivot (IsPivot) and
nonnegative values. 7.1: uniform_price_unit_demand_dominant (3
bidders, 2 units, all Rat values and bids, so every grid),
demand_reduction (decide over examples/multiunit.lang muAu) and
multi_unit_vickrey_dominant (the VCG instance at Fin 3 with marginal
values and the rival valuing the remaining units). 7.3:
two_object_vcg_dominant and two_object_vcg_ir (the two-bidder VCG
instance at Fin 4 with complementary bundles), revenue_not_monotone,
vcg_outcome_not_in_core and
exposure_problem (decide over the examples/combinatorial.lang
witnesses). No kernel change, no new refusal, no mutants. Not in this
slice: the explicit 2-bidder allocation to the highest nonnegative
virtual value under a regular prior (its monotonicity is the IsMonoNat
hypothesis; the 4-type check is mcOptICIR and mcRevIsVS in
examples/mechanism.lang) and the overflow items (3.4 for all Rat bids,
the 3-bidder second price dominance, the 3.1.4 entry fee closed form).
The L4 welfare regressions check every outcome of the multi-unit and
two-object instances, guarding against counting the same goods twice.
Current state: 10 proof files, 95 proof refusals, 1345 gate tests (the
14 gate rows).

M5 slice L5 is done (2026-10-12): the Ch 4, Ch 2 and App A ports, as
proof terms in test/proof/ok/l5.lang (65 theorems, 28 defs; 14
theorems are copies from l1.lang, 8 from l1b.lang and 12 from l3.lang;
the examples/commonvalue.lang defs are copies by line number). Ch 4:
winners_curse_grid, on every signal grid the conditional mean of a
nondecreasing value given a win (the rival signal below m) is at most
the unconditional mean, stated cross-multiplied for nonnegative
weights (l5LowerLe and l5UpperGe bound the two partial sums by the
value at m, two sumRat_split transports and one cert close it);
winners_curse_uniform (s + s/2 < s + 1/2 for s < 1, all Rat s);
winners_curse_uniform_grid, winners_curse_discrete and
wallet_naive_bid_loses_grid (decide over the commonvalue witnesses).
Ch 2: wallet_first_price_best_reply and wallet_second_price_best_reply
(the bids z and 2z are best replies for all Rat signals, a square
slack), the grid instances and wallet_revenue_equivalence. App A:
sumRat_mul_sumRat (the product rule), expect_linear, expect_const,
expect_indep_mul, total_expectation (field with the marginal as the
divisor, the cvMeanV shape), max_cdf_two (P(max < k) = P(X < k)^2 on
any grid by the indicator product l5IndMax and l3SumBelow),
uniform_max_two_grid and uniform_min_two_grid (the exact sums
(4K^3 - 3K^2 - K)/6 and (2K^3 - 3K^2 + K)/6 of the max and the min of
two uniform draws on a grid of size k, by natInd on the row
recurrences) and their means (2/3 and 1/3 in the limit, N/(N+1) and
(N-1)/(N+1) for N = 2). No kernel change, no new refusal, no mutants.
Not in this slice: the NatLe m n form of the grid curse, the
continuous naive bid loss, total expectation over a flat joint, order
statistics for N > 2 and the overflow items of L4. Current state: 11
proof files, 95 proof refusals, 1346 gate tests (the 14 gate rows).

M5 slice L6 is done (2026-10-12): the overflow turn. test/proof/ok/l6.lang
holds 46 theorems: 40 helpers copied by line number from l5.lang and l1.lang,
and 6 new ones: winners_curse_grid_le (the NatLe m n form), total_expectation_flat
(a flat joint on Fin (natMul m n)), wallet_naive_bid_loses (every Rat signal s with
0 < s < 1), vickrey3_truthful_dominant and vickrey3_truthful_best_response (three
bidders, the rival pair read through finMax), entry_fee_closed_form (the fee at
cutoff c on the uniform grid of K values is c * c / K). No kernel rule was added.
Current state: 12 proof files, 95 proof refusals, gate 1347 checks, 0 failures.

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
- A value line of `read`, `eval` or `verify` has no size limit, and its
  depth limit is 2000 levels (`src/front/eval.c:10`, `SPEC.md` O18). The
  printer prints the last argument of each operation in a loop, so a
  long list does not add levels. A value that nests 2000 levels in the
  other arguments, for example `pair (pair (... 0) 0) 0` with 2000
  levels of `pair`, is refused with exit 1 and no stdout:
  `EVAL_PRINT_DEPTH` in `eval`, `READ_PRINT_DEPTH` in `read` and
  `verify`. The gate group `limits` checks lines of 1999 and 2000 levels.
  The cut at 200 levels stays for a type (`src/front/eval.c:9`; the
  writer refuses such a type, see above) and for a value in a diagnostic,
  which prints "..." past 200 levels.

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
