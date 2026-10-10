# Host capability probe: tcc-json

Date: 2026-10-08. Host tree: the tcc-json kit of lang-template, read only.
This repository was generated from it at `cdc754e` (2026-10-07). The probe
ran on this tree at `bd755d4` with the post-A6 edits. Driver:
`build/langc` (built 2026-10-08 by `make check`). Other work loaded the
machine, so each wall time is an upper bound. The host facts come from
`docs/host/CAPABILITY.md`; this file gives them for auction-lang.

To run the probe again: `make check` runs the gate. The gate checks each
refusal of sections 3, 4, 7, 9 and 10 (`test/check/expect.txt`,
`test/parse/expect.txt`). Each cost row of section 6 ran below
`python3 probe/guard.py --rss-mb 2048 --timeout 120 --`, which stops a
command above 2048 MB resident memory or above 120 s. The cost program is
one line, `def m : Matrix N N := matId N`. The probes of sections 4 and 8
are small files. All probe files were written below a new temporary
directory. The largest process in the probe used 67.6 MB.

## Questions

Each host must answer these questions. Each answer is one numbered section
below. An answer sets the status of a former in `formers/tcc-json.md`.

| Question | Section | Formers |
|---|---|---|
| Which commands check a file, print a normal form, build and run? | 1 | all |
| Does the host take several files, or must the driver join them? | 2 | all |
| Which recursion does the host accept, and how does it refuse the rest? | 3 | F6, F7 |
| Which `Nat` primitives exist? Does `Nat` have an eliminator? | 4 | F6, F7 |
| How does a result leave the host? | 5 | target |
| What do check and build cost (time and resident memory) at N = 100 and N = 1000? | 6 | all |
| Can a family take a parameter? Can a constructor of such a family appear in a term? | 7 | F4, F12, F15 |
| Can a program project a Sigma? Does the kernel accept Sigma eta? | 8 | F11 |
| How does a program use equality: transport, cong, an index type? | 9 | F12, F13 |
| How does the host report an axiom? | 10 | R2 |

## 1. Commands

| Job | Command | Result |
| --- | --- | --- |
| Check | `build/langc check FILE` | exit 0 and `ok`; exit 1 and `langc: CODE: NAME: message` on stderr |
| Print normal form | `build/langc eval FILE NAME [ARGS...]` | the normal form of NAME on stdout, for example `1200` for `offered` in `examples/sigma.lang` |
| Build | `build/langc build FILE [-o OUT]` | one JSON document on stdout, or in the file OUT |
| Run | none | the JSON document is the result; there is no run step |

- `build/langc` with no command prints
  `langc: USAGE: -: expected a command and a program path` and the usage
  of the three commands.
- One program file for each run.
- `langc eval` of an instance that traps prints `trap` on stdout and the
  trap on stderr, and exits 0. `langc build` refuses that instance with
  exit 1 (section 4).
- Not probed on 2026-10-08: a program that reads the JSON document.
  Milestone M2 adds this (`SPEC.md` section 10, slices C1 to C3).
  `langc read DOC` reads the document and prints each instance. With
  `--read DOC`, `langc eval` and `langc build` make each instance of the
  document a definition of the program. Thus a program can use the
  instances of a read document (`docs/host/README.md`, Queries).

## 2. Several files

A run reads one program file. The grammar has no import form: a file is a
list of `def`, `axiom` and `family` declarations
(`src/front/parser.c:1-11`). The Makefile joins the four domain files into
`build/domain-all.lang`, and `gen/embed.c` makes that file part of the
executable (`docs/host/CAPABILITY.md:14-21`). Thus a program uses the
domain names with no import.

## 3. Recursion and totality

A program cannot recurse by name. `def rec` parses, and the checker
refuses it:
`langc: REFUSE_REC: loop: def rec is refused (rule R3); use fold or unfold`
(`test/check/rec.lang`). A definition can use only earlier definitions. A
use of itself or of a later definition is refused:
`langc: REFUSE_REC: loop: loop is this definition or a later one; a definition can use only earlier ones (rule R3)`
(`test/check/self.lang`, `test/check/later.lang`). Recursion comes from
`fold` over Nat, List and each family, and from `unfold` with a step
limit (`examples/algebra.lang`). `allFin` and `sumRat` iterate below a Nat
bound (`docs/host/CAPABILITY.md:31-48`). The evaluator stops at a depth of
2000 and after 20,000,000 steps for each instance
(`docs/host/CAPABILITY.md:123-129`).

## 4. Nat

`Nat` is an unsigned 64-bit integer (`docs/host/CAPABILITY.md:25`). The
primitives are natAdd, natSub, natMul, natDiv, natMod, natEq, natLe, natLt
and natMax (`src/front/check.c:136` and the lines after it). The checker
refuses natMin, natGe and natGt with `REFUSE_ALLOW`
(`test/check/natmin-use.lang`, `test/check/natge-use.lang`,
`test/check/natgt-use.lang`).

The output of `langc eval` on a probe file:

- `natSub 2 5` is `0`.
- `natDiv 7 0` is `trap`, with `EVAL_DIV_ZERO` on stderr.
- `natAdd 18446744073709551615 1` is `trap`, with `EVAL_OVERFLOW` on
  stderr.
- `langc build` of the same file exits 1:
  `langc: EVAL_DIV_ZERO: b: the instance value traps (a division or modulo by zero)`.

`Nat` has an eliminator: `fold step start n` applies step n times. An
example is `square` (`examples/algebra.lang:4`). The largest value that
leaves the host is 18446744073709551615. `langc build` writes it as a JSON
number: `{"name":"big","type":"Nat","value":18446744073709551615}`. A
larger literal is refused:
`langc: LEX_NAT_RANGE: -: FILE:1:16: a Nat literal is larger than 2^64 - 1`
(`test/parse/nat-range.lang`).

## 5. How a result leaves the host

`langc check` prints `ok` on stdout. `langc eval` prints a normal form on
stdout. `langc build` writes one JSON document on stdout, or in OUT with
`-o OUT`. The document is `{"auction-lang":1,"instances":[...]}`, with one
object for each instance: name, type and value. The value of a Sigma is an
object with `witness` and `payload`, and a payload of `Eq` type is `null`
(probe on `examples/sigma.lang`). A refused build writes no partial
document and no `-o` file (`docs/host/CAPABILITY.md:136-137`). Each error
is one line on stderr, `langc: CODE: NAME: message`, and the exit code is
1.

## 6. Cost

The program is `def m : Matrix N N := matId N`, a matrix of N^2 cells.
Each cell is the time and the peak resident memory of the command, from
`probe/guard.py`.

| Step | N = 100 | N = 1000 |
| --- | --- | --- |
| Check | 0.08 s, 12.3 MB | 0.19 s, 12.6 MB |
| Print normal form | 0.05 s, 14.5 MB, 50201 bytes of text | 1.48 s, 30.0 MB, 5002001 bytes of text |
| Build | 0.12 s, 15.2 MB, 180280 bytes of JSON | 2.57 s, 67.6 MB, 18002082 bytes of JSON |
| Run | no run step | no run step |

Other sessions loaded the machine, so a wall time can vary by 2x to 4x.
A matrix with more than 2^25 cells is a trap (`EVAL_MATRIX_SIZE`,
`docs/host/CAPABILITY.md:61-62`). The smallest matId that refuses is
matId 5793 (`test/emit/mat-size.lang`).

## 7. Parametric families

A program cannot declare a family:
`langc: REFUSE_DATA: Coin: a program cannot declare a family (rule R1); only domain/domain.lang can`
(`test/check/data.lang`). The domain families take Nat parameters:
`Lot (size : Nat)` (`domain/domain.lang:8`) and
`OpenGame (x : Nat) (s : Nat) (y : Nat) (r : Nat) (m : Nat)`
(`domain/opengame.lang:8`). Their constructors appear in terms, with the
parameters as the first arguments: `makeLot 3 250`
(`examples/inventory.lang:9`) and `makeGame x s x s 1 ...`
(`domain/opengame.lang:11`). The parameters are part of the type, for
example `Lot 3` and the m of an open game
(`test/check/opengame-middle.lang`). Not probed: a family with a type
parameter. The domain has none, and a program cannot declare one.

## 8. Sigma projection and eta

`witness` and `payload` check (`examples/sigma.lang:4-5`). A program
writes the type as `Sigma (n : Nat) (Eq Nat n price)` or as
`(n : Nat) * Eq Nat n price` (`examples/sigma.lang:3`, `:6`), and the
value as `pack 1200 refl`. There is no Sigma pattern: the grammar has no
pattern form (`src/front/parser.c:1-11`).

The kernel has no Sigma eta. For a closed `s`, `refl` proves
`Eq (Sigma (n : Nat) (Eq Nat n 3)) (pack (witness s) (payload s)) s`,
because both sides normalize to the same pack (probe: `ok`). For a
variable `s`, the checker refuses the same equality:
`langc: TYPE_REFL: etaOpen: refl cannot prove the equality: the left side normalizes to pack (witness s) (payload s), the right side normalizes to s`.

## 9. Equality

The family is `Eq A a b`. The proofs are `refl`, `symm e`, `trans e1 e2`,
`transport P e u` and `cong f e` (`examples/equality.lang:4-9`). One
family serves each type: `Eq Nat` in `examples/equality.lang` and
`Eq (Sigma ...)` in the probe of section 8. `refl` proves an equality
only when both sides have the same normal form. Otherwise the checker
refuses it with `TYPE_REFL` (`test/check/bad-refl.lang`). Only the former
examples use `Eq` (`examples/equality.lang`, `examples/laws.lang`,
`examples/sigma.lang`). The auction examples state each result as a value,
and the gate compares it (`test/eval/expect.txt`).

## 10. Axiom detection

No command lists the axioms of a file. The grammar has
`axiom NAME : TYPE` (`src/front/parser.c:3`), and the checker refuses it:
`langc: REFUSE_AXIOM: magic: an axiom has no proof (rule R2)`
(`test/check/axiom.lang`). A file with no axiom gives `ok`. Thus a checked
program has no axiom.

## Consequences for M0

1. A definition cannot recurse by name. It iterates with `fold`,
   `unfold`, `allFin` or `sumRat`, or it uses a matrix operation
   (section 3).
2. Equality proofs can quantify over variable sizes and values. For
   example, `fun (n : Nat) => refl` checks at type
   `(n : Nat) -> Eq Nat n n`. Sigma eta fails for a variable because its
   two sides do not normalize to the same term (sections 8 and 9).
   The auction examples compute results at fixed sizes, and the gate
   compares their values.
3. The families of the auction library live in the domain files (section
   7, rule R1). A program uses them and cannot add one.
4. The matrix size sets the cost (section 6). The limit of 2^25 cells
   bounds the memory and the time (`docs/host/CAPABILITY.md:61-62`,
   `SPEC.md` O4).
5. A checked program has no axiom (section 10). Each result is computed.
