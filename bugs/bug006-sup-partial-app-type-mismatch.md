# Bug: internal type mismatch during search on a well-typed Sledgehammer problem

**File:** `bugs/bug006-sup-partial-app-type-mismatch.p` (unminimized — full
Sledgehammer output, 1621 lines, isa/deeper/B-mesh-th0/train120k corpus,
`HOL-Library/0027_Confluence/prob_00035_001401`)

**Trigger:** `bash bugs/bug006-sup-partial-app-type-mismatch.sh` — minimized
option set (see *Minimization* below). The original strategy that found this
(31 flags, 6-term heuristic) is preserved as
`bugs/bug006-sup-partial-app-type-mismatch-original.sh`.

**Status:** PARTIALLY FIXED. Root-caused to generic eta-reduction producing
a bare, unapplied `$eq` (see *Root-cause investigation* below) — fixed in
`TERMS/cte_lambda.c`. That fix alone eliminated the original `Type error`
message shown below but uncovered further crashes in the same derivation,
tracked separately as **`bug009-pdtree-ho-match-prefix-assertion`** (own
`.p`/`.sh`/`.md`, HO discrimination-tree matching, not sup/type-mismatch) —
this bug (bug006) is followed by bug009. Three fixes across both bugs are
now verified together on the *minimized* 6-flag strategy
(`bugs/bug006-sup-partial-app-type-mismatch.sh`), which no longer reproduces
either the original `Type error` or any of bug009's assertions, on any
build. **The *original*, unminimized 31-flag strategy
(`bugs/bug006-sup-partial-app-type-mismatch-original.sh`) is clean on a
debug (`-O0`) build but still crashes on a release (`-O03`) build** — a
fourth occurrence of the same assertion class, not yet fixed; see bug009's
`.md`, "Not yet done". Options
minimized down to 6 flags + a 2-term heuristic; `.p` input itself not
minimized.

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

## Root-cause investigation (2026-09-15)

Debugged under gdb against a `-O0` debug build (`Makefile.vars` OPTFLAGS
temporarily changed from `-O03 -fomit-frame-pointer`; asserts were already
enabled either way — `NODEBUG` has no `-DNDEBUG`). Under `-O0`, this
reproducer doesn't hit the `Type error` message above at all — it crashes
*earlier*, on `assert(var->type == bind->type)` in `SubstAddBinding`
(`cte_subst.h:102`), reached from `SubstComputeMguHO` →
`NextCSUElement`/HO unification during paramodulation
(`CONTROL/cco_paramodulation.c`). The `-O03` build's `Type error` and the
`-O0` build's assertion are the same underlying defect, just caught at two
different points (the release build lets the ill-typed substitution through
and `TypeInferSort` catches it later; the debug build's assert catches the
substitution attempt itself).

**The two terms being unified at the crash** (`orig_lhs`/`orig_rhs` from
`NextCSUElement`'s debug-only fields):
```
orig_lhs:  sup_su270570050_a_o_o @ Y5 @ (equiv_equivclp_a @ (bNF_eq_onp_a @ esk62_0)) @ Y6 @ Y7
orig_rhs:  sup_su270570050_a_o_o @ (sup_su270570050_a_o_o @ X5 @ X9) @ X9 @ X6 @ X7
```
Both are `$o`-typed applications of the same head symbol, so the top-level
unification call is legitimate; decomposing argument-by-argument is also
the textbook-correct thing to do. The failing bind is at argument slot 2:
`X9` (type `T = (a>a>$o)>(a>a>$o)>$o`, forced by its *other* occurrence
inside `orig_rhs`, as `sup`'s 1st-argument partial-application `sup(X5,X9)`)
against `equiv_equivclp_a @ (bNF_eq_onp_a @ esk62_0)` (type `S = a>a>$o`).
Checked directly: `X9`'s two occurrences in `orig_rhs` are the literal same
term cell (same pointer, same type) — not an occurs-check/cyclic-binding
issue, and not a tail-chasing substitution bug. `orig_rhs`'s repeated `X9`
is simply the shape of the `sup.right_idem` axiom (`sup(sup(A,B),B) =
sup(A,B)`) it's an instance of — legitimate by design. `orig_lhs` is
individually ill-typed on its own, though: its argument 2 needs type `T`
per `sup_su270570050_a_o_o`'s own declared signature, but holds an `S`-typed
term. Confirmed directly — writing that exact shape into a `.p` file by
hand (`join @ Y5 @ k @ Y6 @ Y7`, `k : S`) gets rejected immediately by the
*parser*'s own `TypeInferSort` with the identical "Type mismatch in
argument #2" message. So a term shaped like `orig_lhs` can never be
constructed directly/validly — some internal (non-parser) code path built
it without going through that same check.

**Where `esk62_0` actually comes from — not what it looks like.** It has no
defining `∃x.Φ` at all; it isn't a genuine Skolem witness. gdb-traced its
creation (`SigDeclareType` breakpoint, filtered by `f_code`) to
`OCBFindMinConst` (`ORDERINGS/cto_ocb.c`) — the term-ordering module's
"give me some minimal-weight constant of this type" helper, called from
`subst_complete_min_instance`/`instance_is_rule` (`CLAUSES/ccl_rewrite.c`),
which only runs when `--strong-rw-inst` is on (confirmed: `ocb->rewrite_
strong_rhs_inst` is exactly that flag). When an equation's RHS has a
variable the match left unbound, this machinery fills it with an arbitrary
existing (or freshly-fabricated) constant of the right type *purely to
decide whether the rewrite orients correctly* — and then, in
`term_is_top_rewritable`, that same arbitrary witness gets reused to build
the actual replacement term (`TBInsertInstantiated`/`TermAddRWLink`),
becoming permanent, real clause content. Logically defensible only if the
equation genuinely doesn't depend on that variable's value; not
independently verified here.

**Where the type violation itself is seeded.** In a user-supplied `-l2`
trace (`bug006-sup-partial-app-type-mismatch.l2.out`, not committed —
regenerate with `-l2` added to the `.sh`), the malformed
`sup(_, S-typed, _, _)` shape already exists as ordinary clause content
from clause `c_0_11942` onward (out of ~58469 total) — far before the final
crash, not something built transiently in the last step. `c_0_11942` itself
(`sup_su270570050_a_o_o @ X5 @ $eq @ X6 @ X7 | X6 != X7`) is well-typed:
`$eq` there is E's genuinely-polymorphic built-in equality symbol,
correctly typed `T` for that occurrence (comparing two `a>a>$o` relations).
Somewhere between `c_0_11942` and the crash, that `T`-typed `$eq` gets
replaced by the `S`-typed `equiv_equivclp_a @ (bNF_eq_onp_a @ esk62_0)` —
not traced to the exact single paramodulation/rewrite step (the chain is
thousands of clauses long), but the first appearance of *bare* (unapplied)
`$eq` anywhere in the run was traced precisely: clause `c_0_1250`
(line 1251, ~2% into the run), from `split_conjunct` clausifying
`strong_confluentp_def` (Confluence.thy:13-14) applied to `r` — completely
mundane, the very definition the whole proof is about. Along the way
`^[Z0,Z1]:(Z0=Z1)` (the identity relation written as a lambda) gets
eta-reduced down to the bare primitive `$eq` symbol.

**That eta-reduction is the actual bug.** `TypeInferSort`
(`cte_typecheck.c:250-256`) explicitly rejects a bare, unapplied `$eq`/
`$neq` ("Equality must have at least one argument") when parsing input —
because `$eq`'s type is derived from its argument at each use, not stored
globally, so an unapplied occurrence is inherently type-ambiguous. Five
call sites in the unification/matching machinery (`cte_match_mgu_1-1.c` x2,
`cte_pattern_match_mgu.c` x2, `cte_ho_csu.c`) specifically check
`SigIsPolymorphic()` to guard against exactly this ambiguity — but every
one is naturally gated by `arity != 0`, since they check an *already
applied* occurrence. Generic eta-reduction
(`TERMS/cte_lambda.c`, `reduce_eta_top_level`/`do_eta_reduce_db`) has *zero*
occurrences of `SigIsPolymorphic` — it's purely structural (de Bruijn
index matching), with no concept of "this symbol needs an argument to be
well-typed." So `λZ0.λZ1.($eq @ Z0 @ Z1)` eta-reduces, two ordinary steps,
straight down to bare `$eq` — producing exactly the shape every other part
of the codebase treats as needing special care, and the one shape
`TypeInferSort` itself rejects.

Minimal reproducer (no crash, just demonstrates the bare-`$eq` artifact):
`bugs/bug006-eta-reduce-bare-eq.p` + `.sh` — before the fix, `-l2` output
contains `p @ $eq` (bare); confirmed via gdb that `reduce_eta_top_level`
computes `to_drop=2` for `matrix = $eq @ db(1) @ db(0)` (flat, non-phony,
`arity=2`), stripping both arguments.

**Fix applied**: `TERMS/cte_lambda.c`,
`reduce_eta_top_level` — after computing `to_drop`, cap it so a
`SigIsPolymorphic` head is never reduced past its last argument:
```c
long total_applied = matrix->arity - (TermIsPhonyApp(matrix) ? 1 : 0);
FunCode head_fc = TermIsPhonyApp(matrix) ? matrix->args[0]->f_code : matrix->f_code;
if(to_drop >= total_applied && total_applied > 0 &&
   head_fc > 0 && SigIsPolymorphic(bank->sig, head_fc))
{
   to_drop = total_applied - 1;
}
```
(`head_fc > 0` guards against calling `SigIsPolymorphic` on a variable head
— an applied-free-variable case tripped `cte_signature.c:500: SigIsPolymorphic:
Assertion 'f_code > 0' failed` without it.) **Verified**: bare `$eq` is gone
from `bug006-eta-reduce-bare-eq.sh` output (now `p @ (^[Z0:a]:($eq @ Z0))`);
a sanity check with an ordinary (non-polymorphic) symbol confirms eta-
reduction is otherwise unaffected — still collapses fully to a bare head as
before. Quick regression pass over several `EXAMPLE_PROBLEMS` (`--auto`,
various): no new crashes, normal `SZS status` results.

**Effect on this bug**: the original `Type error`/`SubstAddBinding`
assertion is gone. But a **different** assertion now fires, *earlier* in
the same run (~0.33s / ~4094 clauses generated, vs. ~3.2s / ~58469 clauses
before the fix):
```
eprover-ho: cte_match_mgu_1-1.c:439: SubstComputeMatchHO: Assertion
`res == MATCH_FAILED || TermStructPrefixEqual(s, t, DEREF_ONCE, DEREF_NEVER, res, sig)' failed.
```
Call chain: `pdtree_forward` → `SubstMatchComplete` → `SubstComputeMatchHO`
— discrimination-tree-indexed **demodulator matching**
(`PDTreeFindNextDemodulator`), a different subsystem than the paramodulation
unification the original bug hit. At the point of failure: pattern `s = X5`
(bare free variable, type `T`) against target `t = λY. ord_less_eq_a_a_o
(λZ. ($@_var @ Y @ Z))` (a lambda, also type `T`, with an applied free
variable buried inside). This should take the immediate-bind fast path;
instead it recurses into `t`'s structure (the deepest job pair at the
actual assertion failure is `db(1)` matched against itself). Not
root-caused as part of *this* investigation — plausibly a **pre-existing,
latent bug in `SubstComputeMatchHO`** (matching a bare variable against a
non-`DB`-closed/lambda-containing target) that the eta-reduction fix's
changed term shapes simply exposed for the first time in this derivation,
rather than something the fix itself introduced logically. Not confirmed
either way.

**Followed by `bug009-pdtree-ho-match-prefix-assertion`**, opened to track
this second crash on its own: isolated it into a self-contained ~450-line
reproducer (`bugs/bug009-pdtree-ho-match-prefix-assertion.p`/`.sh`, no
special flags needed, crashes in ~0.03s CPU) by gdb-dumping the exact clause
being forward-contracted and the full 111-clause demodulator index active
at the crash point from a live run, and feeding all of it back to E as
axioms. Two smaller isolation attempts (a hand-picked plausible pair; then
the exact two real clauses involved, `i_0_452`/`i_0_1703`) both ran clean in
isolation — the bug needs the accumulated discrimination-tree state (build
order/topology across the full demodulator set), not just the content of
the two clauses that appear in the final backtrace. See that bug's own `.md`
for further investigation — this doc's remaining "not yet done" items are
about the *original* type-mismatch bug, which the eta-reduction fix already
resolves.

## Fix verification (2026-09-15)

All three fixes — this bug's eta-reduction fix, and bug009's two matching/
unification fixes (leftover-argument `DB`-closedness, and the bare-symbol
rigid-rigid type check; see bug009's `.md` for details) — applied together.
Checked on **both** a debug (`-O0`) and release (`-O03`, the repo's normal
`Makefile.vars` flags) build, since the first round of verification (`-O0`
only) turned out to be a false negative for one case below — `-O0` is slow
enough that it didn't reach deep enough into the search within the same
time budget to find everything the much-faster release build does.

- **`bugs/bug006-sup-partial-app-type-mismatch.sh`** (minimized 6-flag
  strategy): no `Type error`, no assertion, `SZS status ResourceOut` — on
  **both** builds.
- **`bugs/bug006-sup-partial-app-type-mismatch-original.sh`** (the original
  31-flag / 6-term-heuristic strategy that first found this, run against the
  same unminimized `.p`): clean on `-O0`. **Still crashes on `-O03`** — a
  fourth occurrence of the `SubstComputeMatchHO` assertion class (bug009's
  territory now, not this bug's original `Type error`), not yet fixed. (A
  user-supplied log from a *different*, RamParILS-tuned strategy —
  `ram-ec9bb3fd89992e15`, tripping on `esk508_0` instead of `esk62_0` —
  turned out to have the exact same flags as this original script once
  fully pasted, so it's covered by the same result.)
- **Regression pass**: all 34 `EXAMPLE_PROBLEMS` (2 pre-existing, unrelated
  missing-include failures aside) — normal `SZS status` results, no crashes,
  both builds. `bugs/bug001`–`bug005`, `bug007`, `bug008` reproducers — all
  still behave as documented, no regressions. `bugs/bug009-pdtree-ho-match-
  prefix-assertion.sh` — no crash on either build, `ResourceOut`.

## Not yet done

- **This bug's own `Type error`/original `SubstAddBinding` crash is fixed
  and verified on both builds** — but the underlying problem (the same `.p`,
  same original strategy) still crashes via a different mechanism (a 4th
  `SubstComputeMatchHO` occurrence, release build only). See bug009's `.md`
  for that investigation; not yet fixed.
- No minimization of the `.p` input — still the full 1621-line Sledgehammer
  file as generated. Flag-level minimization is done (see above); problem-level
  minimization is not.
- Not confirmed whether other instances in the same corpus hit the same
  symbol/pattern or a different one — this is the one instance found so far.
- No fix has been reported upstream.
- Not run to completion with a large CPU budget — verification so far only
  confirms clean `ResourceOut` within the strategies' own (short, 5-25s)
  time limits, not that either strategy actually finds a proof of this
  problem given more time.
