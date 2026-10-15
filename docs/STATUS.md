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
chapters 3 to 7. Milestone M5 (proofs, the slices K1 to K3b and L1 to
L6) adds the `Prop` sort, the `theorem` form, 72 kernel rules and 12
proof files with 413 theorem checks. `README.md` `## Book coverage`
names the results proved for all sizes and the results not covered.

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

M5 is integrated (2026-10-12, session 13). `README.md` has the new column
`Proved for all sizes (M5)` in `## Book coverage`. It names the proof file and
the theorems for each book section. `SPEC.md` section 11 is final, and
`docs/VALIDATION.md` has the M5 counts. There are 12 proof files in
`test/proof/ok` with 413 theorem checks: the K files have 88 (k1 15, k1a 4,
k2 39, k3 16, k3b 14) and the L files have 325 (l1 26, l1b 49, l2 33, l3 51,
l4 55, l5 65, l6 46). A file cannot import another file. The files identify
their copied helpers. The gate has 1347 checks in 14 main groups:
31 examples, 10 parse refusals, 85 check refusals,
12 proofs, 95 proof refusals, 578 evals, 31 json builds, 15 build refusals,
31 round trips, 395 reads, 29 read refusals, 6 verify refusals, 15 limits and
14 queries. It also runs 21 C typed reader and 9 C certificate regressions,
for 1377 checks in total and 0 failures. The kernel mutant sweep kills 90 of 92
mutants: k1 28 of 30
(mutants 24 and 25 are equivalent, USER ruling 2026-10-12, the rows stay),
k2 27 of 27, k3 17 of 17, k3b 15 of 15 and l1b 3 of 3, with 0 build failures.
The 85 M4 mutants run again on the M5 kernel, and all 85 are killed. Every
proof JSON is byte-equal to its golden (the gate compares each one with
`cmp`). Not built in M5: the Ch 5 and App C affiliation definitions and finite
lemmas (turn 11 of the plan slipped) and the items under `Not covered after
M5` in `README.md`.

M6 session 1 is done (2026-10-13): the kernel batch. J (K1b): no code
change. A rule application is always a chain of `VAL_APP` values on a rule
constant with 0 arguments, because `OP_ARITY[OP_RULE]` is 0
(`src/front/eval.c:62`) and `arity_ok` refuses a different count. A stuck rule
with arguments is never built. `kernel_app_type` types that chain, so a `def`
of type `And`, `Or` or `Exists` is not an instance. `test/proof/ok/k1.lang`
has three such defs (`k1.json` stays empty), and `test/read/prop.json` checks
that the reader refuses the type `And True True` (`READ_TYPE`). K (K3a):
`examples/arith.lang` has the instance `smaller`, which calls `natMin`. M(i):
`test/emit/fuel.lang` reaches `EVAL_FUEL` and `test/emit/depth.lang` reaches
`JSON_DEPTH` (see Known limits). L (K3b) is not built in session 1 (see the
M6 session 1b paragraph). The gate has 1352 checks in 14 main groups
(evals 579, build refusals 17, reads 396 and read refusals 30; the other
groups are as above), 1382 checks with the C regressions, and 0 failures. The
k1 mutant sweep kills 31 of 33 (mutants 24 and 25 are equivalent), so the
kernel sweep kills 93 of 95.

M6 sessions 1b and 1c are done (2026-10-13): L (K3b), the divisor that is
not a literal (USER ruling R4 (b), form b1). `ring` now takes
`(H : Prop) -> H`, as `field` does. Each of the 160 `ring` calls in the
proof tests adds `True trivial`. In `ring`, `field`, `cert` and `certLt`,
each divisor `d` that is not a literal needs the fact `Ne Rat d 0/1` in
`H`, else the rule is refused (`NF_DIVISOR`). The rules no longer make an
atom of a `ratDiv` term. `cert` and `certLt` read `H` in two passes, so a
comparison can divide by `d` when the `Ne` fact for `d` comes after it.
A `cert` remainder `N / D` with `N = 0` is the constant 0 (ruling R8 (a);
see Known limits). `test/proof/ok/k3b.lang` has 3 new theorems (416
theorem checks in total), and there are 3 new proof refusals (98 in
total). Every other proof `.out` and `.json` file is byte-equal to
e1cbd8e. The gate has 1355 checks in 14 main groups, 1385 checks with the
C regressions, and 0 failures. The k3b mutant sweep kills 21 of 21 and
the k1 sweep kills 31 of 33, so the kernel sweep kills 99 of 101.

M6 session 2 is done (2026-10-13): Ch. 5 and App. C (slice D).
`test/proof/ok/l7.lang` has 24 theorem checks: 10 copied helpers and 14 new
theorems. Its new defs are `IsMonoFin`, `IsAffiliated2`, `l7AffAt` and
`l7AffFlag`. `IsAffiliated2` is affiliation of two signals on a finite grid
as a Prop. `l7AffFlag` is the same condition as a Flag, and the bridge
theorem `l7AffiliatedOfFlag` lets `decide` check a chosen prior. The
correlated prior of `affiliated.lang` is affiliated, the anti-correlated
prior is not, and every product prior is affiliated. The finite lemmas are
the Chebyshev sum inequality (the Harris inequality on a chain), the
increasing conditional expectation, and MLRP implies FOSD of the
conditionals. The last two are in cross-multiplied form. There is no new
kernel rule, and `src/front/nf.c` and `src/front/check.c` do not change, so
there is no mutant run. Not built: the Harris inequality for 2 variables
(M7 session 3 closes it in `l14.lang`), App. D, and the division form of the conditional expectation (a `cert`
across a `ratDiv` by a sum hits the R8 (a) limit, see Known limits). The 13
proof files have 440 theorem checks. The gate has 1356 checks in 14 main
groups, 1386 checks with the C regressions, and 0 failures.

M6 session 3 is done (2026-10-13): the explicit 2-bidder allocation of
Ch. 6.4 (slice F). `test/proof/ok/l8.lang` has 47 theorem checks: 34 copied
helpers (the closure of `virtual_value_auction_attains_bound` from
`l4.lang`, and two `l7.lang` helpers) and 13 new theorems. Its new defs are
`l8H` and `l8W` (the higher nonnegative virtual value wins, and a tie splits
1/2), `l8Q` (the interim allocation of bidder 1), `l8Phi` (the discrete
virtual value) and the instance `l8Inst`. The book theorems are
`two_bidder_allocation_feasible` (the two shares sum to at most 1),
`two_bidder_tie_split`, `two_bidder_interim_monotone` (a nondecreasing
virtual value gives a nondecreasing interim allocation) and
`two_bidder_auction_attains_bound` (regularity, a hypothesis on the
discrete virtual value, gives IC, IR and the expected virtual surplus as
the revenue). `l8InstanceCheck` checks the 4-type instance of
`examples/mechanism.lang` by `decide`: `l8Phi` is `mcPhi`, the instance is
regular, `l8W` is `mcOptq`, `l8Q` is `mcOptQ`, and the revenue is 13/8.
The proofs compare virtual values only through `ratLe` flags, so there is
no division and no `Ne` fact. There is no new kernel rule, and
`src/front` does not change, so there is no mutant run. Not built: a Prop
instance of the main theorem at 4 types (there is no Fin-from-Nat
constructor, so the regularity Prop for an open type needs a Nat case
split; the `decide` check pins the instance), and ironing (M7
sessions 6 and 7 build it in `l16.lang`, with the ironed virtual value
as a hypothesis). The 14 proof
files have 487 theorem checks. The gate has 1357 checks in 14 main
groups, 1387 checks with the C regressions, and 0 failures.

M6 session 4 is done (2026-10-13): no pure symmetric equilibrium in 3.4
for all Rat bids (slice E). `test/proof/ok/l9.lang` has 22 theorem checks:
2 copied helpers (`l1FlagYesNeNo` and `l3NoOfNot` from `l3.lang`) and 20
new theorems. It also copies the defs `dsTick`, `dsShare` and `dsGridPay`
from `examples/discrete.lang`. Its new defs are `l9S` (the share of a bid
against the rival bid: a win gives 1, a tie 1/2 and a loss 0), `l9W` (the
expected share against the pure strategy (bL, bH)), `l9Pay` (the payoff),
`l9Eq` (the equilibrium Prop, a Pi over all deviations z : Rat) and the
grid defs `l9G`, `l9GridAgree`, `l9GridNoEq` and `l9Grid`. The book
theorems are `no_pure_symmetric_equilibrium` (for all Rat vL < vH,
0 < p < 1, bL and bH, the pair (bL, bH) is not an equilibrium) and
`no_pure_equilibrium_half` (vL = 0, vH = 1 and p = 1/2, as in
`discrete.lang`). The proof has five cases, with a = bL and c = bH. If
a = c = b < vH, the high type bids b + (vH - b)/4 (`l9Pool1`). If a = c = b
and vH <= b, the low type bids b - 1 (`l9Pool2`). If a < c < vH, the high
type bids c + (vH - c) p/4 (`l9Up`). If a < c and vH <= c, the high type
bids the midpoint of a and c (`l9Mid`, the only case that uses p < 1). If
c < a, the low type bids c and the high type bids a, and the sum of the two
conditions gives vH <= vL (`l9Down`). This last case applies the Pi two
times (one witness for each type), with no `Exists`. `l9Pos` (a product of
two positive terms is positive) gives each deviation step as a positive
term with a literal factor. `l9GridCheck` uses `decide` to check that
`l9Pay` agrees with `dsGridPay` on the grid k/8 for v = 0 and v = 1, and
that no grid pair is an equilibrium against grid deviations. There is no
new kernel rule, and `src/front` does not change, so there is no mutant
run. Not built: the form with more than 2 values (M7
session 4 closes it in `l15.lang`). The 15 proof files have
509 theorem checks. The gate has 1358 checks in 14 main groups, 1388
checks with the C regressions, and 0 failures.

M6 session 5 is done (2026-10-13): the sequential game form of ch. 7 with
finite witnesses (slice G part 1). `test/proof/ok/l10.lang` has 12 theorem
checks: 2 copied theorems (`l1FlagYesNeNo` and `l9Tri` from `l9.lang`) and
10 new theorems. It also copies the def `l9S` from `l9.lang`, `cbV` from
`examples/combinatorial.lang`, and `muMin`, `muMax`, `muInd`, `muTop`,
`muMed` and `muSeqCellRef` from `examples/multiunit.lang`. The game form has
2 identical objects, 2 rounds of second price and 3 unit demand bidders with
values on the Fin 4 grid, and a round 1 winner leaves. The history h is a
`Flag` (won or lost in round 1). `l10Pay` is the payoff: a winner gets the
value minus the round 1 price, and a loser gets the round 2 share against
the rival bid (a tie splits 1/2) times the value minus the rival bid. A
round 2 strategy is a function b2 of the own value and h. The book theorems
are `sequential_round2_truthful_dominant` (for all b2, Rat values v,
histories h, round 1 prices and rival bids, the payoff of the bid b2 v h is
at most the payoff of the bid v) and
`sequential_nonmonotone_not_equilibrium`. The second theorem uses `decide`
on the chosen round 1 profile `l10B`: the values 0, 1, 2 and 3 bid 0, 1, 1/4
and 5/4. The profile is not monotone (the value 2 bids less than the value
1), the value 1 gains with the bid 1/2, and `l10Eq l10B` (deviations k/4 for
k < 13, truthful round 2 bids) is `flagNo`. `l10AgreeCheck` uses `decide` to
check that `l10Cell` with truthful round 2 bids agrees with `muSeqCellRef`
on all 4096 value and bid cells. `l10SPDom` gives the round 2 step:
`l10SCases` splits each share into its 3 values, and `l10D1`, `l10DHalf` and
`l10D0` split again on the truthful bid. As found (a python search, not a
gate check): on the grid k/4, no symmetric pure round 1 profile (13^4
profiles) is an equilibrium with truthful round 2 bids, monotone or not.
There is no new kernel rule, and `src/front` does not change, so there is no
mutant run. The 16 proof files have 521 theorem checks. The gate has 1359
checks in 14 main groups, 1389 checks with the C regressions, and 0
failures.

M6 session 6 is done (2026-10-13): the package auctions of ch. 7 with finite
witnesses (slice G part 2). `test/proof/ok/l11.lang` has 14 theorem checks:
2 copied theorems (`l1FlagYesNeNo` and `l9Tri` from `l9.lang`) and 12 new
theorems. It also copies the defs `cbV`, `cbMax`, `cbLocal`, `cbPay1` to
`cbPay3`, `cbRev` and `cbExpRev` from `examples/combinatorial.lang`. Bidder
1 bids a on the object A, bidder 2 bids b on the object B, and bidder 3 bids
s on the package AB. The feasible allocations are k : Fin 5 in the order 0 =
A to bidder 1 and B to bidder 2, 1 = AB to bidder 3, 2 = A to bidder 1 only,
3 = B to bidder 2 only, and 4 = nothing sold. `l11W` is the reported welfare
(the bid sum). `l11M1` to `l11M4` are the running maximum: the next
allocation in the order replaces the incumbent only when its bid sum is
higher. `l11Win` is the argmax with the same comparisons, so a tie goes to
the first allocation in the order. Each winner pays its bid (`l11Has1` to
`l11Has3`; `l11Rev` is the sum). The book theorems are
`package_wd_maximizes_welfare` (for all Rat a, b, s and every k : Fin 5,
`l11W a b s k` is at most the welfare of `l11Win a b s`) and 3 `decide`
checks on the grid `cbV`: `package_wd_agrees_vcg_allocation` (the allocation
is 0 when `cbLocal` holds and 1 when it does not, the same as VCG, and
`l11Rev` is `l11M4`), `package_truthful_not_equilibrium` (the values are
i.i.d. on `cbV` with probability 1/4 each: truthful bids give each type the
interim utility 0, the synergy type 3 gains 3/16 with the bid 2, and `l11Eq`
is `flagNo`) and `package_revenue_vs_vcg` (at truthful bids, `cbRev` is at
most `l11Rev` on all 64 cells, `cbRev 3 3 3` is 0 against `l11Rev` 6, and
the expected revenue is 207/64 against `cbExpRev` = 25/32; truthful bids are
not an equilibrium, so this compares the bid sums only). `l11WinW` gives the
main proof step: the welfare of the chosen allocation is `l11M4`. `l11Up1`
to `l11Up3` bound each running maximum by `l11M4`. There is no new kernel
rule, and `src/front` does not change, so there is no mutant run. The 17
proof files have 535 theorem checks. The gate has 1360 checks in 14 main
groups, 1390 checks with the C regressions, and 0 failures.

M6 session 7 is done (2026-10-14): the N-ary profile layer (slice H part 1,
rulings H1 = a, H2 = a and H3 = a by default). A profile of n bids on the
grid `Fin k` is a value of `Fin (natPow k n)`, and the bid of bidder i is
digit i in base k (bidder 0 is the low digit). New in `src`: the Nat
operation `natPow` (`OP_NAT_POW`: `natPow x 0` is 1, x below 2 has no
loop, and an overflow traps), the built-ins `finProj` (4 arguments) and
`finUpd` (5 arguments), `fin_power` (`TYPE_FIN_SIZE` when `natPow k n`
traps) and the rule codes `RULE_FIN_PROJ` and `RULE_FIN_UPD`.
`kernel/rules.lang` has 6 new rules after `pairVal`: `powZero`,
`powSucc`, `projUpd`, `projUpdNe`, `updProj` and `profExt` (78 kernel
rules). `test/proof/ok/k4.lang` has 19 theorem checks, and there are 8 new
proof refusals (one wrong argument per new rule code, `TYPE_FIN_SIZE` at
`finProj 2 64` and `TYPE_MISMATCH` for a bid of type `Fin n` at
`finUpd`). The k3 mutant list has 15 new rows (4 in `eval.c`, 5 in
`check.c` and 6 in `kernel/rules.lang`). The k3 sweep kills 32 of 32,
and the k1 rerun kills 31 of 33 (rows 24 and 25 are equivalent and survive, as before). The kernel sweep kills
114 of 116. The 18 proof files have 554 theorem checks. The gate has
1369 checks in 14 main groups, 1399 checks with the C regressions, and 0
failures.

M6 session 8 is done (2026-10-14): the App A product rule for N factors
(slice H part 2a, ruling H4 = a by default). New in `src`: the built-in
`prodRat` (`OP_PROD_RAT`: the product of `f i` over `i : Fin n`, and
`prodRat 0 f` is 1/1; it shares `rule_all_fin` with `allFin` and `sumRat`)
and the rule codes `RULE_PROD_RAT_ZERO`, `RULE_PROD_RAT_SUCC` and
`RULE_SUM_PROD`. `kernel/rules.lang` has 3 new rules after `sumRatSucc`:
`prodRatZero`, `prodRatSucc` and `sumProd` (81 kernel rules).
`test/proof/ok/k5.lang` has 16 theorem checks, and there are 4 new proof
refusals (one wrong argument per new rule code and `TYPE_MISMATCH` for a
function of type `Fin 2 -> Nat` at `prodRat`). The k3 mutant list has 8
new rows (3 in `eval.c`, 1 in `check.c` and 4 in `kernel/rules.lang`). The
k3 sweep kills 40 of 40, and the k1 rerun kills 31 of 33 (rows 24 and 25
are equivalent and survive, as before). The kernel sweep kills 122 of 124.
The 19 proof files have 570 theorem checks. The gate has 1374 checks in 14
main groups, 1404 checks with the C regressions, and 0 failures.

M6 session 9 is done (2026-10-14): the mechanisms for every N and the
order statistics for N > 2 (slice H part 2b, ruling H5 = a by default).
There is no change in `src` or in `kernel/rules.lang`: the highest other
bid is a guest def (`l12Hob`, a fold over the profile, and `l12IsHob`,
the relation). `test/proof/ok/l12.lang` has 11 defs and 36 theorem
checks: the tie-winning second price bound for every N
(`l12SecondPriceDominant`, and `l12Vickrey3` for N = 3), truthful dominance
for the Rat utility at fixed sizes (`l12SpDominant33` and
`l12SpDominant24`), and five tie regressions for 2 and 3 bidders. The Rat
utility follows the library rule that ties go to the lowest bidder index.
Further checks cover the first price win probability and expected payoff for N iid bids
(`l12FirstPriceWin` and `l12FirstPricePay`), and the CDF of the second
highest bid (`l12SecondForm` and `l12OneAbove`). There are no new proof
refusals and no new mutant rows. The 20 proof files have 606 theorem
checks. The gate has 1375 checks in 14 main groups, 1405 checks with the
C regressions, and 0 failures. Two items stay closed checks at fixed
sizes: a general proof that `l12Hob` and `l12IsHob` agree needs an
`allFin` intro rule, and the split of the second highest bid event for
every N needs a count lemma over `Fin n`. M7 session 2c closes both (`l13.lang`).

M6 is done (2026-10-14, session 10, ruling C1 = a, no kernel change).
The kernel has 81 rules, and the 20 proof files have 606 theorem checks
(480 for the book and 126 for the kernel rules). The gate has 1375
checks in 14 main groups, 1405 checks with the C regressions, and 0
failures. The kernel mutant sweep kills 122 of 124 mutants (2 equivalent
k1 mutants). The two closed-check items of session 9 stay as limits.
Appendix B, the exercises, the results for a general continuous F, the
continuous "as found" results and bignum `Nat` and `Rat` stay out of
scope (rulings R1 = a and R3 = a). The close record is the M6 paragraph
at the end of `SPEC.md` section 10.

M7 session 1 is done (2026-10-14): the kernel rule `allFinIntro` (H3
part 1, ruling H3a = a). It is the converse of `allFinSpec`: a proof of
`(i : Fin n) -> Holds (p i)` gives `Holds (allFin n p)` for the same n
and p. `test/proof/ok/k6.lang` has 12 theorem checks, and 5 new proof
refusals check a wrong n, a wrong p and a proof at one fixed i. The
kernel has 82 rules, the 21 proof files have 618 theorem checks, and the
gate has 1411 checks with 0 failures. Session 2 uses the rule for the
general proofs of the two C1 limits.

M7 session 2b is done (2026-10-14): the kernel rule `natEqSpec`
(ruling H3c = a). `Holds (natEq a b)` holds if and only if `Eq Nat a b`
holds. `test/proof/ok/k7.lang` has 18 theorem checks, and 5 new proof
refusals check a wrong argument and three false uses at 2 and 3. The
kernel has 83 rules, the 22 proof files have 636 theorem checks, and the
gate has 1417 checks with 0 failures. Session 2c uses the rule for the
general proofs of the two C1 limits (`l13.lang`).

M7 session 2c is done (2026-10-14): the general proofs of the two C1
limits in `test/proof/ok/l13.lang` (56 theorem checks, no kernel
change). `l13HobIff` proves that the relation `l12IsHob` and the fold
`l12Hob` agree for each k, n, x, i and m. `l13CntNat` is the count lemma
(ruling H3b = a), and `l13SecondSplit` is the split of the second
highest bid event for each k, n, x and t. The 23 proof files have 692
theorem checks, and the gate has 1418 checks with 0 failures. H3 is
done. Session 3 is D2 (the Harris inequality for 2 variables).

M7 session 3 is done (2026-10-14): the Harris inequality for 2
variables on a product prior in `test/proof/ok/l14.lang` (22 theorem
checks, no kernel change, ruling D2a = a). `harris_two_var` proves it on
the grid `Fin m` by `Fin n` with the prior p i times q j, and
`harris_grid` proves it on `Fin k` by `Fin k`. The proof uses the
Chebyshev sum inequality on each row and on the row sums. The 24 proof
files have 714 theorem checks, and the gate has 1419 checks with 0
failures. D2 is done. Session 4 is E2 (3.4 for k values).

M7 session 4 is done (2026-10-14): no pure symmetric equilibrium in 3.4
for k values in `test/proof/ok/l15.lang` (30 theorem checks, no kernel
change, ruling E2a = a). `no_pure_symmetric_equilibrium_k` proves it for
k = n + 2 values in strict order, a prior with positive masses that sum
to 1, and all bid functions `Fin k -> Rat`. The proof uses a top bid
(`l15ArgMax`, by `natInd`). The 25 proof files have 744 theorem checks,
and the gate has 1420 checks with 0 failures. Session 5 is E2 part 2.

M7 session 6 is done (2026-10-14): ironing in 6.4 in
`test/proof/ok/l16.lang` (64 theorem checks, no kernel change, ruling
F2a = a). The ironed virtual value is a hypothesis. `ironing_bound`,
`ironing_tight`, `ironed_auction_attains_bound` and
`ironing_revenue_bound` hold for every type count N = n + 1, and
`l16InstanceCheck` checks a 4-type prior that is not regular by
`decide` (revenue 91/128 for each bidder). Session 5 closed E2 at
session 4 (ruling E2b = b) with no change in the repo. The 26 proof
files have 808 theorem checks, and the gate has 1421 checks with 0
failures. Session 7 is F2 part 2: the optimality among symmetric 2-bidder
feasible rules and the general theorem on the witness.

M7 session 7 is done (2026-10-14): F2 part 2 in
`test/proof/ok/l16.lang` (20 new theorem checks, 84 in the file, no
kernel change, ruling F2b = a). `two_bidder_ironed_surplus_bound` and
`ironed_auction_optimal` prove that the ironed auction is optimal among
the IC and IR mechanisms with a symmetric 2-bidder ex-post feasible
allocation and independent types from the same prior, for every type
count N = n + 1. Optimality for asymmetric allocations is not proved.
The 4-type witness now has its
conditions as Prop facts for each k (a Nat case split), and
`l16WitnessOptimal` gives the revenue bound 91/128 from the general
theorem. The 26 proof files have 828 theorem checks, and the gate has
1421 checks with 0 failures. Session 8 is the M7 close.

M7 is done (2026-10-14, session 8, scope (a): H3, D2, E2 and F2, no
kernel change in the close). The kernel has 83 rules, and the 26 proof
files have 828 theorem checks (672 for the book and 156 for the kernel
rules). The gate has 1391 checks in 14 main groups, 1421 checks with
the C regressions, and 0 failures. The kernel mutant sweep kills 131 of
133 mutants (2 equivalent k1 mutants). The two C1 limits of M6 are
closed. Limits that stay: the ironed virtual value is a hypothesis (no
convex hull), and the optimality of the ironed auction is proved only
for a symmetric 2-bidder allocation with independent types from the
same prior. Appendix B, the exercises, the results for a general
continuous F, the continuous "as found" results and bignum `Nat` and
`Rat` stay out of scope (rulings R1 = a and R3 = a). The close record
is the M7 paragraph at the end of `SPEC.md` section 10.

M8 session 1 is done (2026-10-14): F3, the optimality of the ironed
auction for asymmetric 2-bidder allocations (rulings F3a = a and
F3b = a, no kernel change). `l16.lang` gains 4 theorem checks
(`l16SymPair`, `l16Pair`, `ironed_auction_optimal_asym` and
`l16WitnessOptimalAsym`), so the 26 proof files have 832 theorem checks
(676 for the book and 156 for the kernel rules). The two allocations x1
and x2 use one prior f for both bidders. The gate has 1391 checks in 14
main groups, 1421 checks with the C regressions, and 0 failures. The
ironed virtual value stays a hypothesis until F4 (sessions 2 to 4).

M8 sessions 2 to 4 are done (2026-10-14): F4 parts 1 to 3, the convex
hull that gives the ironed virtual value (rulings F4a = a with 7
sessions, and F4c = a; no kernel change). The new file `l17.lang` has 48
theorem checks (23 new). `ironing_exists` proves that a phibar with the
hypotheses H1, H2a, H2c and H2b of `ironed_auction_optimal` exists for
each prior f with f k >= 0 and f k > 0 below N. `l17NoHull` shows that
f k >= 0 alone is not sufficient. The 27 proof files have 880 theorem
checks (724 for the book and 156 for the kernel rules). The gate has
1392 checks in 14 main groups, 1422 checks with the C regressions, and 0
failures. F4 ends with session 5: the optimality theorems with the
built phibar, so phibar is not a hypothesis.

M8 session 5 is done (2026-10-15): F4 part 4, the optimality theorems
with the built phibar. `ironed_auction_optimal_built` and
`ironed_auction_optimal_asym_built` take hf and hpos (f k > 0 below N)
and no phibar; `ironing_exists` gives phibar. The witness rows apply them
at the 4-type prior of M7 session 6. `l17.lang` has 107 theorem checks
(29 new in M8). The 27 proof files have 939 theorem checks (783 for the
book and 156 for the kernel rules). The gate has 1392 checks in 14 main
groups, 1422 checks with the C regressions, and 0 failures. F4 is done.
Session 6 is V: the 85 M4 mutants and the kernel sweep.

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
  (`src/json.h:15`). `test/emit/depth.lang` reaches the JSON depth
  limit (`JSON_DEPTH`): a `Stack` of 2000 `push` cells, with the type `Stack`.
- The evaluator has a fuel of 20,000,000 steps for each instance
  (`src/front/core.h:18`). `test/emit/fuel.lang` reaches the fuel limit
  (`EVAL_FUEL`, 0.28 s): a `fold` of 10,000,000 turns, about 2 steps each.
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
- The `cert` and `certLt` remainder `N / D` must have `N = 0`, or `N` and
  `D` must both be constants (`SPEC.md` section 11.2, ruling R8 (a)). The
  normal form does not cancel a common factor of `N` and `D`. Thus a
  remainder that is a nonzero constant only after that cancellation is
  refused with `RULE_CERT`, for example
  `cert (ratDiv x y) (ratAdd (ratDiv x y) 1/1) (Ne Rat y 0/1) h`.

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
