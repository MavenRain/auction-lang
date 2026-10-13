# Validation

Date: 2026-10-11. TinyCC: 0.9.28rc 2026-09-04 mob@0fb54300 (AArch64
Darwin). Apple clang: 21.0.0 (clang-2100.0.123.102). Host executable:
`build/langc` from the sources of slice E1, the last commit that changes
`src/`. Slices B1 to B3, B5, C4, D5 and D6 change no source file. Slice B4
changes no file in `src/`, but it changes `domain/opengame.lang`, which
`build/langc` embeds. Slice D2 changes `src/` and
`domain/opengame.lang`. Slice D3 changes the host sources `src/main.c`,
`src/read.c` and `src/read.h`. Slice D4 changes the host sources
`src/front/eval.c:2247,2251`, `src/front/core.h`, `src/front/check.c`,
`src/read.c:274,785` and `src/json.c:10,381-387`. Slice E1 changes
`src/front/eval.c`, `src/front/core.h`, `src/front/check.c` and
`src/read.c`.

`make check` passes. It builds `build/langc` with TinyCC
(`-std=c99 -Wall -Werror`). It checks the sources with clang
(`-Wall -Wextra -Wswitch-enum -Werror -fsyntax-only`). Then `test/gate.sh`
runs 1241 tests with zero failures: 31 examples, 10 parse refusals, 86 check
refusals, 578 eval lines, 31 JSON builds, 15 build refusals, 31 round
trips, 395 reads, 29 read refusals, 6 verify refusals, 15 limits checks
and 14 queries. It also runs
`build/read-typed-test`: 16 typed reader and printing regressions with zero failures.
The gate also checks a nesting of 1100 parentheses, the usage exit code 2,
the `-o` file, a `Fin` argument out of range, the reader on white space,
escapes, 2008 levels of nesting and a document of 16 MiB, the document of
`langc build --read`, the absence of em-dashes and en-dashes in the kit,
and Node `JSON.parse` of each golden. A timed run of the E1 gate
(`/usr/bin/time -l make check`) took 29.62 seconds real and 19.76
seconds user. Its maximum resident set size was 775,946,240 bytes (740.0
MiB). Other jobs loaded the machine before this run (load 4.3 to 5.4).

The base commit cdc754e (2026-10-07) makes the language from lang-template.
It has 10 examples, 9 parse refusals, 19 check refusals, 2 build refusals,
25 eval lines and 10 JSON goldens. They check the formers F1 to F15 on the
sample domain.

Slice A1 (22698b8) adds 51 test files, and `examples/arith.lang`,
`examples/fin.lang` and `examples/rat.lang`. It adds 1 parse refusal, 42
check refusals, 5 build refusals, 114 eval lines and 3 JSON goldens. They
check `Nat`, `Flag`, `Fin n` and `Rat`. The refusals cover the wrong uses of
these types.

Slice A2 (10cfe36) adds 21 test files, `domain/finstoch.lang` and
`examples/matrix.lang`. It adds 17 check refusals, 3 build refusals, 44 eval
lines and 1 JSON golden. They check the `Matrix` type, its operations and
the structure maps of FinStoch.

Slice A3 (3c35793) adds 7 test files, `domain/auction.lang` and
`examples/auction.lang`. It adds 3 check refusals, 3 build refusals, 28 eval
lines and 1 JSON golden. They check the auction library.

Slice A4 (d9c699f) adds 9 test files, `domain/opengame.lang`,
`examples/opengame.lang` and `examples/bayesnash.lang`. It adds 5 check
refusals, 2 build refusals, 47 eval lines and 2 JSON goldens. They check the
open games and the auction kernels.

Slice A5 (50ac45c) adds 3 test files (the JSON goldens),
`examples/utility.lang`, `examples/revenue.lang` and
`examples/dominance.lang`. It adds 51 eval lines. They check the utility,
revenue and dominance results. A5 adds no refusal.

Slice A6 adds no test. It fills `SPEC.md`, `docs/STATUS.md`, this file and
`README.md`.

Slice B1 adds no test file. It adds 22 definitions to 4 examples and 22
eval lines: 12 for `examples/utility.lang`, 2 for `examples/revenue.lang`,
2 for `examples/auction.lang` and 6 for `examples/opengame.lang`. They use
the 22 domain definitions that no example or test used before B1. The JSON
goldens of these 4 examples get the new instances.

Slice B2 adds the example `examples/finstoch-laws.lang`, its JSON golden
and 15 eval lines. Each of 12 definitions checks one FinStoch law of
auction-cat (`MarkovCat/FinStoch.lean`) with `matEq` at a fixed size. The
laws are: the left identity, the right identity and the associativity of
`matComp`; the triangle (sizes 2 and 3); the pentagon (sizes 2, 2, 2 and 2,
with 16 x 16 matrices); the hexagon (sizes 2, 3 and 4, with 24 x 24
matrices); the braiding symmetry (sizes 3 and 4); the coassociativity, the 2
counit laws and the cocommutativity of `matCopy` (size 3); and the
interchange of `matComp` and `matKron`. Each law evaluates to 1. The 3 other
definitions are the stochastic matrices for the laws that take morphisms
(2 x 3, 3 x 2 and 2 x 2). These matrices are not identities.
`examples/matrix.lang` checks some of these laws at other sizes. A pair
index has the first component as the low digit. Thus each associator is an
identity matrix, and the pentagon and the triangle check the reindex maps
and `matKron`, but no permutation.

Slice B3 adds the example `examples/truthful.lang`, its JSON golden and 11
eval lines. Each definition checks one auction-cat result of O6 (`SPEC.md`
section 9) at a fixed size, and each definition evaluates to 1.
`vickTruth3` and `vickTruth5` check `vickrey_truthful_dominant` over each
value and each 2 bids at n = 3 and 5. `vickResTruth3` and `vickResTruth5`
check `vickreyReserve_truthful_dominant` also over each reserve. A utility
is a `Fin n` value, so these checks use `natLe` on `finVal`.
`spsb3ResBidder3Dom3` checks `spsb3Reserve_bidder3_kernel_dominance` at
n = 3 over each reserve, each constant bid and each value profile. The
deviator applies the bid only to the value of bidder 3. Thus the constant
bids cover each bid function. `revGe2` to `revGe5` check
`expectedRevenue_fpsb_ge_spsb` with `ratLe` at n = 2, 3, 4 and 5, with the
prior weight v + 1. `fpsbKernel4` checks the 2-bidder first price kernel
form with `matEq` at n = 4 (`examples/opengame.lang` checks n = 2 and 3).
`fpsb3ResKernel2` checks the 3-bidder first price kernel form with a
reserve at n = 2 for each reserve. No check stops on the evaluation fuel of
20,000,000 steps. A mutant of each kind of check (the 2 sides swapped, or
the kernel function of the other auction) evaluates to 0, so no check is
vacuous.

Slice B4 makes the size count of `auctionGame3` at n = 3 (O7). At n = 3
the kron form has a 5,832 x 5,832 matrix. That is 34,012,224 cells, over
the limit of 33,554,432. The factored games `auctionGame3Det` and
`auctionGame3Deviator1Det` to `auctionGame3Deviator3Det` build the view
and the update as the `matOfFn` of the Fns of `Vickrey3.lean`. Their
largest matrix is the kron of the identity of size 27 and the mechanism:
729 x 5,832, or 4,251,528 cells. The 3-bidder scores use these games. The
new eval lines `spsb3FnEq 3`, `fpsb3FnEq 3` and `spsb3Dev3FnEq 3` check
the score kernels at n = 3. `update3DevFnEq 2` checks that the deviator
games of the kron form have the update of `auctionGame3`. With
`view3FnEq`, `update3FnEq` and `view3Dev1FnEq` to `view3Dev3FnEq`, it
links the kron games and the factored games at n = 2. The trap line
`spsb3FnEq 4` and `test/emit/opengame-size.lang` refuse at n = 4. One
factored kernel at n = 3 took 11.75 s, with a maximum resident set of
217,317,376 bytes, and it did not stop on the fuel.

Slice B5 adds no test. It closes M1 in `SPEC.md`, `docs/STATUS.md` and
this file. The gate counts are the counts of B4.

Slice C1 (a96d772) adds the reader `src/read.c` and the verb `langc
read`. It adds 19 test files in `test/read/` and 2 count lines (O11 a):
22 round trips, one for each JSON golden, and 18 read refusals. The gate
makes the large documents in a temporary directory. Two mutants of the
reader fail the gate: a write loop that drops the last item of an array,
and a version test that also takes version 2. The gate took 43 s.

Slice C2 (3a1397a) adds the typed read. It adds 10 refusal documents in
`test/read/` and 10 read refusals (28 in all). The new count line `reads`
checks 254 instances: for each instance of each JSON golden, the read
value equals the value from `langc eval`. `test/read-typed.c` adds 13
typed reader regressions: higher-universe types, wide matrix sums, type
isolation and name controls. Two mutants fail the gate: a Rat cell with no
lowest-terms test, and a Matrix decode that drops one row. The gate took
146.89 s on a quiet machine and 341.75 s at load 73.

Slice C3 (1c3128f) adds the queries. It adds 10 test files in
`test/query/`: 5 programs, 4 refusal documents and `expect.txt`. The
count line `queries` checks 14 lines: 9 query values over
`test/json/matrix.json`, `test/json/revenue.json` and
`test/json/auction.json`, 3 `REFUSE_NAME` refusals and 2 `READ_NAME`
refusals. Two checks have no count: `langc build --read` writes the 4
instances of the program only, and `langc eval PROG --read` with no
document exits 2. Two mutants fail the gate: a bind loop that drops the
last instance, and a name test that ignores the read names. The gate took
85.19 s at load 12 to 18.

Slice C4 adds no test and changes no source file. It closes M2 in
`SPEC.md`, `docs/STATUS.md`, this file, `probe/CAPABILITY.md` and
`README.md`. The gate counts are the counts of C3. The reads section
calls `langc eval` 254 times, and each call checks the example again.
Thus the gate time is an M3 item.

Slice D1 adds no test (O13 a, first part). `matComp`
(`reduce_mat_comp` in `src/front/eval.c`) skips a term when its left cell
is 0. `cell_mul` of 0 and a cell gives 0/1 and never traps, so the skip
changes no value and removes no `EVAL_OVERFLOW` trap. The gate counts are
the counts of C3. Each of the 3 slowest eval lines ran alone with
`/usr/bin/time -p` (seconds real, before and after the skip):
`opengame spsb3FnEq 3` 5.76 and 1.06, `opengame fpsb3FnEq 3` 6.60 and
1.15, `opengame spsb3Dev3FnEq 3` 6.97 and 1.00 (load 12 to 14). The gate
took 46.72 s at load 13 to 17. The gate of C3 took 85.19 s at load 12 to
18.

Slice D2 adds the core operation `matIdKronComp m y r s K U` (O15 a). Its
value is the value of `matComp (natMul m y) (natMul m r) s (matKron m m
y r (matId m) K) U`, but it makes no kron. Cell (x, j) is the sum over
c < r of K (x div m, c) times U (c m + x mod m, j), with c in ascending
order, and a term with a K cell of 0 is skipped as in D1. These are the
nonzero terms of the kron form in the same order, so each value and each
`EVAL_OVERFLOW` trap is the same. `gameScore` uses it.
`examples/matrix.lang` gets 7 eval lines: 4 compare it with the kron
form (`idKronCompEq` with the sizes 2 2 3 2, 3 2 2 3, 2 3 2 2 and
1 1 1 1), 1 prints a 4 x 2 result, and 2 check `EVAL_MATRIX_SIZE`. The
3-bidder scores get eval lines at n = 4 (`spsb3FnEq 4` and
`spsb3Dev1FnEq 4` to `spsb3Dev3FnEq 4`) in place of the refusal line
`spsb3FnEq 4`, and `test/emit/opengame-size.lang` checks the refusal at
n = 6. Thus the eval lines go from 361 to 371. No JSON golden changes.
Each line ran alone with `/usr/bin/time -p` (seconds real, load 5 to
6): the 3 slow lines of D1 take 0.10, 0.10 and 0.09 (D1: 1.06, 1.15
and 1.00), and the 4 lines at n = 4 take 1.10, 1.04, 1.09 and 1.17. At
n = 5 the size count fits, but `spsb3FnEq 5` stops at the arena limit
(`langc: OOM`, exit 1, 0.78 s). A mutant that swaps the kron order in
the U row makes `idKronCompEq 2 2 3 2` 0, and the gate fails on 8
lines. The gate took 28.51 s at load 5 to 6.

Slice D3 adds the verb `langc verify PROG DOC` (O17 a; `run_verify` in
`src/main.c`, `read_verify` in `src/read.c`). It reads DOC with the
checks of `langc read`, and it evaluates each instance of PROG once. For
each instance of PROG, in program order, it compares the read type and
value with the eval type and value. Then it looks for a read instance
that PROG does not have, and it compares the bytes of DOC with the bytes
that `langc build PROG` writes. A difference is a refusal with exit 1
and one of 4 codes, in this order: `VERIFY_MISSING`, `VERIFY_VALUE`,
`VERIFY_EXTRA` and `VERIFY_BYTES`. When each comparison holds, it prints
the lines of `langc read`. The new group `test/verify/` has 4 fixtures.
Each is `test/json/functions.json` with one change: a changed value, a
removed instance, an added instance and 1 added space.
`test/verify/expect.txt` gives the code of each. The gate also runs
`langc verify` with no DOC (exit 2, `USAGE`) and with a DOC that does
not exist (exit 2, `IO`). Thus there are 6 verify refusals and 896
tests. The reads section (O13 a, second part) calls `langc read` and
`langc verify` once for each of the 22 goldens, in place of 254 calls of
`langc eval`. Each output must have one line for each instance, and each
read line must be equal to the verify line at the same position. Thus
the gate compares the full line (name, type and value), and the count
stays at 254 reads. Each read value must also match a no-argument eval
expectation in `test/eval/expect.txt`, checked independently by the eval
group. Review adds the 46 missing expectations, bringing that group to
417 lines. The reads loop before review ran alone with `/usr/bin/time -p`:
9.46 s real before D3 and 2.18 s after it (load 9). Two mutants fail
the gate: a verify with no value comparison gives `VERIFY_BYTES` in
place of `VERIFY_VALUE` for the changed value (1 failure), and a gate
that removes the first read line from the comparison fails on 40 lines.
The gate before review took 23.90 s at load 8 to 10. A review mutant
that makes `langc eval examples/arith.lang odd` print 999 instead of 1
passes the initial D3 gate but fails the prior gate. With the restored
eval expectations it fails the reviewed gate. The reviewed gate passes
all 896 tests and the 13 typed reader regressions.

Slice D4 makes `langc read` and `langc eval` print each line in full
(O14 a). Before D4, a read line or an eval line longer than 64 KiB
printed with a "..." cut and exit 0, and the writer could cut a long
`type`. `value_print_len` counts the text of a value, and `value_text`
prints it into an arena buffer of that size
(`src/front/eval.c:2247,2251`). The writer measures the type before it
evaluates the instance. A type longer than 4096 bytes, or a type that
nests 200 levels, is `JSON_TYPE_SIZE` (exit 1, `src/json.c:10,381-387`).
The reader refuses a decoded `type` longer than 4096 bytes with
`READ_TYPE_SIZE` (exit 1, `src/read.c:274,784`), and
`test/read/expect.txt` adds 1 read refusal. The new gate group `limits`
has 11 checks. `test/limits/long.lang` has a List and a product that
each hold `matId 128`. Their eval lines (82,185 bytes for the List) and
their read lines (up to 82,218 bytes) must be equal in full to the lines
that awk writes, and `langc verify` must print the read lines. The group
builds and reads back a type of 4096 bytes and refuses a type of 4097
bytes. It does the same for types of 199 and 200 levels. Thus there are
908 tests. The group ran alone in 0.47 s real. Two mutants fail the
gate: a `value_text` that caps the buffer at 65536 bytes (4 failures)
and a writer with no type length check (1 failure). The gate took 25.21
s at load 3.3 to 4.4. A value line still has the depth cut of the
printer at 200 levels (`docs/STATUS.md`, Known limits).
Review of D4 adds three C regressions for printing fuel. With two steps
remaining, evaluating `def f : (Nat -> Nat) -> Nat := fun g => 7` must
print `fun g => 7`, as on f7f6741. The initial D4 size pass consumed
the fuel first and the render pass printed `fun g => ?` with exit 0.
Measuring now preserves the fuel for rendering. With only one step,
the command refuses with `EVAL_FUEL` and no output instead of printing
a placeholder. For `def p : Sigma (n : Nat) Nat := pack 0 7`, the
writer starts with exhausted fuel to check its instance budget reset.
The initial D4 writer emitted `Sigma (n : Nat) ?` with exit 0; the
reviewed writer resets the budget before measuring the type and emits
the complete type, as on f7f6741. Measurement and rendering failures
propagate their diagnostics. All three tests fail on the initial D4
tree and pass with the fixes. There are now 16 C regressions.

Slice D5 adds the n = 3 eval lines of the 6 kernels of O16. It changes no
file in `src/` and no file in `domain/`. `test/eval/expect.txt` adds
`spsb3Dev1FnEq 3` and `spsb3Dev2FnEq 3` (lines 274-275). Slice D5 adds 4
closed definitions to `examples/opengame.lang` (lines 49-52):
`res3FnEq3` and `res3Dev1FnEq3` to `res3Dev3FnEq3`. They are the n = 2
definitions with n = 3 and the reserve 1. The eval lines of these
definitions are at `test/eval/expect.txt:255-258`. Each expected value is
1. Each line says that a kernel matrix of the domain equals the `matOfFn`
of its Fn. The auction-cat theorems give this equality for each n (the
comment at `domain/opengame.lang:137-141`). Each of the 6 lines took 0.09
s alone (sandbox, `/usr/bin/time -p`, load 12). Before D1 and D2, O16
gave about 115 s for the 6 lines. `test/json/opengame.json` is written
again with `langc build` and has 18 instances in place of 14. Thus the
counts are 423 eval lines and 258 reads (918 tests). Two mutants fail the
gate: an expected value of 0 for `res3Dev2FnEq3` (2 failures), and a
`res3Dev1FnEq3` with the Fn of `truthful` in place of `halfShading`. This
definition has the value 0 (5 failures).

Slice D6 adds no test and changes no source file. It closes M3 in
`SPEC.md`, `docs/STATUS.md`, this file, `probe/CAPABILITY.md` and
`README.md`. The gate counts and the timed run are those of D5. In M3 the
gate time went from 267 s at load 27 to 43 (the M3 plan, `SPEC.md` O13)
to 27.32 s at load 6.7 to 6.9 (D5). The loads differ, so the two times
do not give one ratio.

Slice E1 sets the depth limit of a value line (`SPEC.md` O18). The gate
group `limits` has 4 new checks, so 15 (922 tests; typed regressions
stay 16). An eval line of a pair of a function with 1999 levels prints in
full, and at 2000 levels it is refused with `EVAL_PRINT_DEPTH`. A read
line of a hand-written `Sigma` document with 1999 levels of `pair` prints
in full, and at 2000 levels it is refused with `READ_PRINT_DEPTH`. The
other groups do not change. The timed run of E1 is above (29.62 s real,
D5 27.32 s; the loads differ). Three mutants fail the gate (outside the
sandbox, each reverted from a copy and compared with `cmp`): a depth
limit of 200 in place of 2000 (2 failures, the 2 lines of 1999 levels);
no refusal on a cut (2 failures, the 2 refusals); and `EVAL_PRINT_DEPTH`
in place of `READ_PRINT_DEPTH` for a read value line (1 failure).

M4 (book coverage, 2026-10-11). The commits ac567fa, e2f4109, 173c0f3
and 8deca33 add 9 examples, their JSON goldens and 141 eval lines. The
close commit changes docs only. No commit of M4 changes `src/` or
`domain/`, so `build/langc` is the build of slice E1. `make check` after
the close passes with zero failures: 1227 tests, with 31 examples, 564
eval lines, 31 JSON builds, 31 round trips and 395 reads. The 141 new
eval lines are the 137 closed definitions of the 9 examples (each one is
also a read) and the 4 lines `muSeqCellAgrees 0` to `muSeqCellAgrees 3`.
The gate time was not measured.

After the close, a follow-up change removes the unused definition `muR2`
from `examples/multiunit.lang`. It also adds the 14 eval lines with
arguments: `mcSPRev 0` to `mcSPRev 3` and `mcPhi 0` to `mcPhi 3`
(mechanism), and `efFeeRev` and `efFeeRev3` at 0, 1 and 3 (entryfee).
An eval line with arguments is not a read. `make check` then passes with
zero failures: 1241 tests, with 578 eval lines and 395 reads. No JSON
golden changes.

The coverage is the finite models and sampled continuous payoff formulas
listed in `README.md`. The affiliated revenue comparison uses a second
price strategy that fails the equilibrium check; the sequential price
comparison also uses a strategy that fails its equilibrium check. These
goldens do not establish equilibrium revenue rankings or a failure of
the martingale property at equilibrium.

Each headline `Flag` has a value mutant in a scratch copy, outside the
repository, that turns it to 0. All 85 mutants are killed: entryfee 7,
discrete 8, riskaverse 9, commonvalue 11, correlated 9, affiliated 6,
mechanism 15, combinatorial 7, multiunit 13.

All 85 M4 mutants run again on the M5 kernel (2026-10-12, session 13), and
all 85 are killed. The M5 gate has 1347 checks in 14 main groups: examples 31,
parse refusals 10, check refusals 85, proofs 12, proof refusals 95, evals 578,
json builds 31, build refusals 15, round trips 31, reads 395, read refusals 29,
verify refusals 6, limits 15 and queries 14. It also runs 21 C typed reader
and 9 C certificate regressions, for 1377 checks in total and 0 failures.
There are 12 proof files with 413 theorem checks and 95 proof refusals.
The kernel mutant sweep kills 90 of 92
mutants, listed by list: k1 28 of 30, k2 27 of 27, k3 17 of 17, k3b 15 of 15
and l1b 3 of 3, with 0 build failures. The k1 mutants 24 and 25 survive. They
are equivalent (USER ruling 2026-10-12), so their rows stay. The sweep restores
every source file, and `git status` shows no change after it. The gate compares
each proof JSON with its golden by `cmp`, so the JSON is byte-equal.

The cost of each example, measured with `/usr/bin/time -l` (one run, no
sandbox). The size is the maximum resident set size (1 MB = 10^6 bytes).

| Example | `langc check` | `langc build` |
|---|---|---|
| entryfee | 0.02 s, 17 MB | 0.04 s, 56 MB |
| discrete | 0.01 s, 17 MB | 0.05 s, 38 MB |
| riskaverse | 0.01 s, 16 MB | 0.02 s, 23 MB |
| commonvalue | 0.01 s, 17 MB | 0.18 s, 214 MB |
| correlated | 0.01 s, 17 MB | 0.12 s, 175 MB |
| affiliated | 0.01 s, 19 MB | 0.20 s, 310 MB |
| mechanism | 0.01 s, 17 MB | 0.10 s, 127 MB |
| combinatorial | 0.01 s, 16 MB | 0.03 s, 34 MB |
| multiunit | 0.02 s, 18 MB | 1.10 s, 895 MB |

M6 session 1 (2026-10-13) adds 5 checks: evals 579 (the arith instance
`smaller` calls `natMin`), reads 396, build refusals 17 (`test/emit/fuel.lang`
gives `EVAL_FUEL` and `test/emit/depth.lang` gives `JSON_DEPTH`) and read
refusals 30 (`test/read/prop.json`, a kernel Prop type, gives `READ_TYPE`). The
gate has 1352 checks in the 14 main groups, 1382 checks in total, and 0
failures. `test/proof/ok/k1.lang` has three defs of kernel Prop types; they are
not instances, so `k1.json` stays empty and the theorem count stays 413. The k1
mutant list has 3 new rows (the type chain in `kernel_app_type`, the Prop test
in `is_instance` and in `instance_type`), and all 3 are killed: k1 31 of 33,
the kernel sweep 93 of 95, with 0 build failures.

M6 sessions 1b and 1c (2026-10-13) close L (K3b): each divisor that is not a
literal needs a `Ne` fact in `ring`, `field`, `cert` and `certLt`, and `ring`
takes `(H : Prop) -> H`. `test/proof/ok/k3b.lang` has 3 new theorems, so the
12 proof files have 416 theorem checks. There are 3 new proof refusals, 98 in
total. The gate has 1355 checks in the 14 main groups, 1385 checks in total,
and 0 failures. The k3b mutant list has 21 rows (6 new, row 13 changed), and
all 21 are killed. The k1 sweep kills 31 of 33 (mutants 24 and 25 are
equivalent), so the kernel sweep kills 99 of 101, with 0 build failures.
Every proof `.out` and `.json` file is byte-equal to e1cbd8e, except
`k3b.out` (3 new lines).

M6 session 2 (2026-10-13) adds the proof file `test/proof/ok/l7.lang` for
Ch. 5 and App. C. It has 24 theorem checks, so the 13 proof files have 440
theorem checks. `langc check` gives exit 0, `langc build` gives
`{"auction-lang":1,"instances":[]}` because the file has no instance, and
`langc verify` gives exit 0. The gate compares the check output with
`l7.out` and the proof JSON with `l7.json`. The gate has 1356 checks in the
14 main groups (proofs 13, proof refusals 98), 1386 checks in total, and 0
failures. No kernel rule changes, so there is no mutant run, and the kernel
sweep stays at 99 of 101.

M6 session 3 (2026-10-13) adds the proof file `test/proof/ok/l8.lang` for
the explicit 2-bidder allocation of Ch. 6.4. It has 47 theorem checks, so
the 14 proof files have 487 theorem checks. `langc check` gives exit 0,
`langc build` gives one instance (`l8Inst : Flag = true`, the `decide`
check on the 4-type instance of `examples/mechanism.lang`), and
`langc verify` gives exit 0. The gate compares the check output with
`l8.out` and the proof JSON with `l8.json`. Three negative probes on
temporary copies are refused: the tie value 1/3 (`RULE_RING`), the
revenue 13/7 (`RULE_DECIDE`) and the feasibility bound 1/2 (`RULE_CERT`).
The gate has 1357 checks in the 14 main groups (proofs 14, proof refusals
98), 1387 checks in total, and 0 failures. No kernel rule changes and
`src/front` does not change, so there is no mutant run, and the kernel
sweep stays at 99 of 101.

M6 session 4 (2026-10-13) adds the proof file `test/proof/ok/l9.lang` for
3.4, no pure symmetric equilibrium for all Rat bids. It has 22 theorem
checks, so the 15 proof files have 509 theorem checks. `langc check` gives
exit 0, `langc build` gives 4 instances (`dsTick : Rat = 1/8` and the
`decide` flags `l9GridAgree`, `l9GridNoEq` and `l9Grid`, all `true`), and
`langc verify` gives exit 0. The gate compares the check output with
`l9.out` and the proof JSON with `l9.json`. Three negative probes on
temporary copies are refused: the tie value 1/3 in `l9STie`
(`TYPE_MISMATCH`), the witness step 1/2 in place of 1/4 in `l9Pool1`
(`RULE_CERT_LT`: the remainder is not a constant) and the v = 1 payoff in
place of the v = 0 payoff in `l9GridAgree` (`RULE_DECIDE`). The gate has
1358 checks in the 14 main groups (proofs 15, proof refusals 98), 1388
checks in total, and 0 failures. No kernel rule changes and `src/front`
does not change, so there is no mutant run, and the kernel sweep stays at
99 of 101.

M6 session 5 (2026-10-13) adds the proof file `test/proof/ok/l10.lang` for
ch. 7, the sequential game form with finite witnesses. It has 12 theorem
checks, so the 16 proof files have 521 theorem checks. `langc check` gives
exit 0, `langc build` gives 2 instances (the `decide` flags `l10Agree` and
`l10NotMonoNotEq`, both `true`), and `langc verify` gives exit 0. The gate
compares the check output with `l10.out` and the proof JSON with `l10.json`.
Three negative probes on temporary copies are refused: the tie value 1/3 in
the copied `l9S` (`TYPE_MISMATCH` in `l10SCases`), the monotone profile 0,
1/4, 1, 5/4 in `l10B` (`RULE_DECIDE` in
`sequential_nonmonotone_not_equilibrium`) and `muMax z x` in place of `muMed
z x y` in `muSeqCellRef` (`RULE_DECIDE` in `l10AgreeCheck`). The gate has
1359 checks in the 14 main groups (proofs 16, proof refusals 98), 1389
checks in total, and 0 failures. No kernel rule changes and `src/front` does
not change, so there is no mutant run, and the kernel sweep stays at 99 of
101.
