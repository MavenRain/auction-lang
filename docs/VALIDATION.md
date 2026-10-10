# Validation

Date: 2026-10-09. TinyCC: 0.9.28rc 2026-09-04 mob@0fb54300 (AArch64
Darwin). Apple clang: 21.0.0 (clang-2100.0.123.102). Host executable:
`build/langc` from the sources of slice D3, the last commit that changes
`src/`. Slices B1 to B3, B5 and C4 change no source file. Slice B4
changes no file in `src/`, but it changes `domain/opengame.lang`, which
`build/langc` embeds. Slice D2 changes `src/` and
`domain/opengame.lang`. Slice D3 changes the host sources `src/main.c`,
`src/read.c` and `src/read.h`.

`make check` passes. It builds `build/langc` with TinyCC
(`-std=c99 -Wall -Werror`). It checks the sources with clang
(`-Wall -Wextra -Wswitch-enum -Werror -fsyntax-only`). Then `test/gate.sh`
runs 896 tests with zero failures: 22 examples, 10 parse refusals, 86 check
refusals, 417 eval lines, 22 JSON builds, 15 build refusals, 22 round
trips, 254 reads, 28 read refusals, 6 verify refusals and 14 queries. It also runs
`build/read-typed-test`: 13 typed reader regressions with zero failures.
The gate also checks a nesting of 1100 parentheses, the usage exit code 2,
the `-o` file, a `Fin` argument out of range, the reader on white space,
escapes, 2008 levels of nesting and a document of 16 MiB, the document of
`langc build --read`, the absence of em-dashes and en-dashes in the kit,
and Node `JSON.parse` of each golden. A timed run before review restored
the independent eval expectations (`/usr/bin/time -l make check`) took
23.90 seconds real and 16.47
seconds user. Its maximum resident set size was 775,815,168 bytes (739.9
MiB). Other jobs loaded the machine during this run (load 8 to 10).

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
