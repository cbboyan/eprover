# Bug: internal type mismatch during search on a well-typed Sledgehammer problem

**File:** `bugs/bug006-sup-partial-app-type-mismatch.p` (unminimized — full
Sledgehammer output, 1621 lines, isa/deeper/B-mesh-th0/train120k corpus,
`HOL-Library/0027_Confluence/prob_00035_001401`)

**Trigger:** `bash bugs/bug006-sup-partial-app-type-mismatch.sh` — minimized
option set (see *Minimization* below). The original strategy that found this
(31 flags, 6-term heuristic) is preserved as
`bugs/bug006-sup-partial-app-type-mismatch-original.sh`.

**Status:** OPEN — options minimized down to 6 flags + a 2-term heuristic;
`.p` input itself not minimized.

**Reproduced locally** with `eprover-ho` from `PATH` (`E 3.5.1-ho Countess
Grey`, rev `25808ee34733f8641fc5962acc203d453d9dbf1a`) and with this repo's own
`PROVER/eprover-ho` build at `d9d729ada01b025961fed2f98a869963b40ffed9` —
identical message and location both times.

**Reproduction is ASLR-sensitive, and disabling it makes debugging easier.**
4 runs each way, plain `bash bug006-sup-partial-app-type-mismatch.sh` vs.
`setarch $(uname -m) -R bash bug006-sup-partial-app-type-mismatch.sh`:

| ASLR | `esk*_0` | free var (`X*`) | output |
|---|---|---|---|
| on (default) | `esk62_0`, every run | varies: `X6541`, `X6536`, `X6576`, `X6536` | message text otherwise identical |
| off (`setarch -R`) | `esk62_0`, every run | fixed: `X6579`, every run | byte-identical across all 4 runs |

So the Skolem symbol itself (`esk62_0`) was already deterministic either way
— it's the free-variable counter that ASLR perturbs, consistent with the
`eprover-ho`/ramparils finding that disabling ASLR per-process makes E's
internal numbering fully reproducible run-to-run. Not relevant to *whether*
this bug fires (it fires identically both ways), but worth using
`setarch $(uname -m) -R` when debugging it under gdb so breakpoints/counters
land on the same values every time.

## Symptom

Deterministic (same message modulo Skolem/variable numbering) — fails on
every run, with the exact strategy in the `.sh`:

```
% Type mismatch in argument #2 of sup_su270570050_a_o_o @ (conver333746293_a_a_o @ ord_less_eq_a_a_o) @ (equiv_equivclp_a @ (bNF_eq_onp_a @ esk62_0)) @ X6 @ X6581: expected (a > a > $o) > (a > a > $o) > $o but got a > a > $o
eprover: Type error
```

Exit code 3. **Not a crash** — E detects the mismatch and aborts cleanly, in
contrast to the segfaults seen on other strategies run against the same
corpus (unrelated, no shared instance).

## Why this is surprising

The input file is **well-typed**. `sup_su270570050_a_o_o` is not an
internally-generated Skolem symbol (those are named `esk*`/`epred*` in this
build) — it is a genuine input constant, declared once in the problem file:

```tptp
thf(sy_c_Lattices_Osup__class_Osup_001_062_I_062_Itf__a_M_062_Itf__a_M_Eo_J_J_M_062_I_062_Itf__a_M_062_Itf__a_M_Eo_J_J_M_Eo_J_J, type,
    sup_su270570050_a_o_o : ((a > a > $o) > (a > a > $o) > $o) > ((a > a > $o) > (a > a > $o) > $o) > (a > a > $o) > (a > a > $o) > $o).
```

It is Isabelle's overloaded `Lattices.sup_class.sup` (relation-of-relations
`Sup`), monomorphized by Sledgehammer to this one type instance; the
`su270570050` suffix is a hash disambiguating it from the several other
`sup_su*`/`sup_sup*` instantiations also declared in the file (one per
distinct type the problem happens to need).

The error message says argument #2 was supplied as
`equiv_equivclp_a @ (bNF_eq_onp_a @ esk62_0)`. `equiv_equivclp_a` is also
declared in the file:

```tptp
thf(..., type,
    equiv_equivclp_a : (a > a > $o) > a > a > $o).
```

so `equiv_equivclp_a` applied to one argument is a **partial application** of
type `a > a > $o` — a curried 1-of-2 application, not the
`(a > a > $o) > (a > a > $o) > $o` that `sup_su270570050_a_o_o`'s signature
requires there. Every symbol in the offending subterm is declared with a type
consistent with how the *input* uses it; the mismatch only appears in a term
E itself builds during preprocessing/search (superposition into or paramodulation
around `sup_su270570050_a_o_o`, going by the surrounding `ConversepPropagation`
/`equivclp` vocabulary). So this looks like an internal derivation combining
terms under two different, incompatible expectations for how many arguments
`equiv_equivclp_a` (or a term built from it) still needs — the same general
class of bug as bug005's "related" findings (`Type mismatch in argument #1 of
hq @ ...`), but on well-typed input reachable without any lambda-at-formula-
position construction, and it aborts rather than segfaulting.

## Minimization

Greedy one-at-a-time bisection (drop one flag, rerun under `setarch $(uname
-m) -R` for deterministic numbering, check the `Type error` message still
fires) starting from the original 31-flag / 6-term-heuristic strategy.

**Round 1** — dropped each of the 31 flags singly. Still crashed without
`-s -p -R --print-statistics --proof-statistics --tstp-format
--delete-bad-limit --condense --sos-uses-input-types
--literal-selection-strategy --term-ordering --definitional-cnf
--forward-demod-level --order-constant-weight --simul-paramod
--destructive-er[-aggressive] --strong-destructive-er --neg-ext --pos-ext
--ext-sup-max-depth --local-rw --satcheck[-proc-interval]` (24 flags — none
of these matter). 7 flags came back "no crash" when dropped:
`--strong-rw-inst`, `--no-eq-unfolding`, `--order-precedence-generation`,
`--order-weight-generation`, `--lift-lambdas=false`, `--fool-unroll=false`,
`--define-heuristic`.

**Round 2** — re-ran with only those 7: still crashed. Dropped each singly
again: all 7 came back essential (a stable fixed point, not just an artifact
of the greedy order) — no single flag among them is "the" cause; the
combination is jointly required.

**Round 3 — minimizing `--define-heuristic` itself.** The original heuristic
is a weighted sum of 6 clause-weight functions:

```
(2*ConjectureRelativeSymbolWeight(PreferGround,0.5,100,100,100,100,1.5,1.5,1),
 6*ConjectureRelativeSymbolWeight(ByDerivationDepth,0.1,100,100,100,100,1.5,1.5,1.5),
 1*Refinedweight(PreferGoals,3,2,2,1.5,2),
 2*ConjectureRelativeSymbolWeight(PreferNonGoals,0.5,100,100,100,100,1.5,1.5,1),
 2*ConjectureRelativeSymbolWeight(PreferGround,0.5,100,100,100,100,1.5,1.5,1),
 1*FIFOWeight(ConstPrio))
```

Dropping each term singly (keeping the 6 outer flags fixed): `PreferGround`
(both copies), `Refinedweight/PreferGoals` dropped without effect;
`ByDerivationDepth`, `PreferNonGoals`, `FIFOWeight(ConstPrio)` came back
needed. Re-tested with just those 3 — still crashed. Dropped each of the 3
singly again: `FIFOWeight(ConstPrio)` turned out **not** essential once the
other two were already gone (redundant-together, missed by the first pass).
Down to 2 terms:

```
(6*ConjectureRelativeSymbolWeight(ByDerivationDepth,0.1,100,100,100,100,1.5,1.5,1.5),
 2*ConjectureRelativeSymbolWeight(PreferNonGoals,0.5,100,100,100,100,1.5,1.5,1))
```

Dropping either of these two: no crash. Both essential — a fixed point.

**Net result:** minimized from 31 flags + 6-term heuristic down to **6 flags
+ a 2-term heuristic** (`bug006-sup-partial-app-type-mismatch.sh`, ~0.9s to
reproduce with `-s`). Both surviving heuristic terms are
`ConjectureRelativeSymbolWeight`, differing only in `PreferenceType`
(`ByDerivationDepth` vs `PreferNonGoals`) and weight (6 vs 2) — needing two
distinct clause-evaluation queues of that function to coexist is itself
informative: the bug likely needs two different clause-selection orders to
pick different clauses that later combine into the offending term, so a
single-queue heuristic wouldn't reach it.

Not attempted: shrinking the numeric argument lists inside
`ConjectureRelativeSymbolWeight(...)` itself, or pairwise-dropping the 6
outer flags (only single-flag drops were tried at each round, so a
still-redundant pair could remain).

## Not yet done

- No minimization of the `.p` input — still the full 1621-line Sledgehammer
  file as generated. Flag-level minimization is done (see above); problem-level
  minimization is not.
- No root-cause analysis of which inference rule builds the offending term
  (unlike bug005, no debugger session yet, no localized call chain).
- Not confirmed whether other instances in the same corpus hit the same
  symbol/pattern or a different one — this is the one instance found so far.
