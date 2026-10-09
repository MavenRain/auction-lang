# Validation

Date: 2026-10-09. TinyCC: 0.9.28rc 2026-09-04 mob@0fb54300 (AArch64
Darwin). Apple clang: 21.0.0 (clang-2100.0.123.102). Host executable:
`build/langc` from the sources of ed8da0a, the last commit that changes
`src/`. Slices B1 to B3 and B5 change no source file. Slice B4 changes no file in `src/`,
but it changes `domain/opengame.lang`, which `build/langc` embeds.

`make check` passes. It builds `build/langc` with TinyCC
(`-std=c99 -Wall -Werror`). It checks the sources with clang
(`-Wall -Wextra -Wswitch-enum -Werror -fsyntax-only`). Then `test/gate.sh`
runs 516 tests with zero failures: 22 examples, 10 parse refusals, 86 check
refusals, 361 eval lines, 22 JSON builds and 15 build refusals. The gate
also checks a nesting of 1100 parentheses, the usage exit code 2, the `-o`
file, a `Fin` argument out of range, the absence of em-dashes and en-dashes
in the kit, and Node `JSON.parse` of each golden. A timed run
(`/usr/bin/time -l make check`) took 39.89 seconds real and 31.52 seconds
user. Its maximum resident set size was 765,116,416 bytes (729.7 MiB).
Other jobs loaded the machine during this run.

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
