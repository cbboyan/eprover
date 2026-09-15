# Bug: `SubstComputeMatchHO` consistency assertion during discrimination-tree demodulator matching

**File:** `bugs/bug009-pdtree-ho-match-prefix-assertion.p` — isolated, ~450
lines (169 type declarations + 111 clauses + one target clause), not
minimized further.

**Trigger:** `bash bugs/bug009-pdtree-ho-match-prefix-assertion.sh` — no
special flags needed, plain `-s --tstp-format`. Before the fix, crashed in
~0.03s CPU (add e.g. `--cpu-limit=10` when testing post-fix, since the file
has no conjecture and the fixed build will otherwise search unbounded).

**Status:** FIXED. Root-caused (see "Root-cause
investigation" below) and fixed in `TERMS/cte_match_mgu_1-1.c`,
`PartiallyMatchVar` — see "Fix verification". Found while re-testing
`bug006-sup-partial-app-type-mismatch`'s reproducer against a build with
that bug's eta-reduction fix applied — this is a **separate, unrelated
defect** (HO discrimination-tree matching, not sup/type-mismatch); it just
happened to start firing, in that specific derivation, once the eta-fix
changed term shapes enough to reach it. See
`bug006-sup-partial-app-type-mismatch.md`'s "Root-cause investigation"
section for the full backstory — bug006 is followed by this bug.

## Symptom

Deterministic assertion failure, not a `Type error`:

```
eprover-ho: cte_match_mgu_1-1.c:439: SubstComputeMatchHO: Assertion
`res == MATCH_FAILED || TermStructPrefixEqual(s, t, DEREF_ONCE, DEREF_NEVER, res, sig)' failed.
```

Call chain (via gdb backtrace on the original, unisolated run):

```
Saturate → ProcessClause → insert_new_clauses → ForwardModifyClause
  → ClauseComputeLINormalform → eqn_li_normalform → term_li_normalform
  → rewrite_with_clause_set_list → rewrite_with_clause_set
  → indexed_find_demodulator → PDTreeFindNextDemodulator
  → PDTreeFindNextIndexedLeaf → pdtree_forward (ccl_pdtrees.c:661)
  → SubstMatchComplete → SubstComputeMatchHO (cte_match_mgu_1-1.c:439)
```

So this happens during **forward contraction** (demodulating a newly
generated clause against already-known unit equations, before it's kept),
specifically while the perfect discrimination tree (`ccl_pdtrees.c`) is
traversing a **variable node** — `pdtree_forward` calls
`SubstMatchComplete(next->variable, term, subst)` where `next->variable` is
the tree's own generalized/indexing variable at that position, not a
variable belonging to one specific named clause.

## What's being matched at the crash

At the point of failure (`s`/`t` are `SubstComputeMatchHO`'s debug-only
aliases for its original, top-level `matcher`/`to_match` parameters — by
the time the assertion fires, the function's *local* `matcher`/`to_match`
have been overwritten by the deepest recursive job, both `db(1)` matched
against itself):

```
s (original pattern): X5              -- bare free variable, type T = (a>a>$o)>(a>a>$o)>$o
t (original target):  λY. ord_less_eq_a_a_o(λZ. ($@_var @ Y @ Z))
                                       -- a lambda, also type T, with an
                                          applied free variable buried inside
```

Matching a lone free variable against *any* target should take the
immediate-bind fast path in `SubstComputeMatchHO`'s main loop
(`TermIsFreeVar(t1) && TermIsDBClosed(t2) && !OccurCheck(...)` →
`SubstAddBinding` directly, no recursion) — but `t` isn't `DB`-closed (its
bound variables are only meaningful inside the lambda), so the match instead
goes down a different, recursive code path, and *that* path leaves `subst`
in a state `TermStructPrefixEqual` doesn't accept as consistent with the
original `s`/`t` pair. Not narrowed down further than that.

## Isolation

Found via a live gdb session on `bug006-sup-partial-app-type-mismatch.sh`
(rebuilt with that bug's eta-reduction fix). Identified two things from the
crashing frames:

- **The clause being forward-contracted** (`clause` in frame
  `ClauseComputeLINormalform`) — in that run, `i_0_1703`:
  ```
  ![X4827,X4828]: ( sup_su270570050_a_o_o @ ord_less_eq_a_a_o
                     @ (^[Z0:a>a>$o]:($eq @ Z0)) @ X4827 @ X4828 )
                   <=> ( ord_less_eq_a_a_o @ X4827 @ X4828 )
  ```
- **The full demodulator index** active at that moment (`demodulators` in
  frame `indexed_find_demodulator`, dumped via `ClauseSetPrint`) — 111
  clauses, including `i_0_452`, the same fact as `i_0_1703` but in
  point-free (2-argument, un-eta-expanded) form:
  ```
  ( sup_su270570050_a_o_o @ ord_less_eq_a_a_o @ (^[Z0:a>a>$o]:($eq @ Z0)) )
    = ord_less_eq_a_a_o
  ```

Two smaller isolation attempts both ran clean (no crash):
1. A hand-picked plausible pair (`fact_18_sup2CI` + an `i_0_1703`-shaped
   clause).
2. The *exact* two real clauses involved (`i_0_452` + `i_0_1703` shapes,
   written directly).

Only pulling in **all 169 type declarations + all 111 demodulator-index
clauses + the target clause** (everything gdb showed was live at the crash)
reproduced it — confirming the bug depends on the discrimination tree's
accumulated build state (insertion order / topology across the whole
indexed set), not just the two clauses that appear in the final backtrace.
`$eq` had to be written in the input as the double-binder identity lambda
(`^[Z0,Z1]:(Z0=Z1)`) rather than the pre-reduced one-argument form
(`^[Z0]:($eq @ Z0)`) — E's parser rejects the latter directly ("Equality
must have at least one argument") when it appears as literal input text
rather than being produced internally by eta-reduction.

## Root-cause investigation (2026-09-15)

**Why matching descends into `t` at all.** `SubstComputeMatchHO`
(`cte_match_mgu_1-1.c`) calls `PruneLambdaPrefix` unconditionally at the top
of every loop iteration. Since the pattern (`X13`, a bare free variable) is
not a lambda but the target (`t`, a lambda) is, `PruneLambdaPrefix` calls
`eta_expand_otf(bank, &to_match, &matcher)`
(`TERMS/cte_pattern_match_mgu.c:487`):
```c
// Assuming the first argument is a lambda and t2 is not... eta-expand t2
// so it has the same lambda prefix as t1, then trim the lambda prefix of t2.
*lambda_ref     = UnfoldLambda(lambda, dbvars);           // strip t's outer binder
*non_lambda_ref = ApplyTerms(bank, ShiftDB(...), dbvars); // apply X13 to the same fresh var
```
This is deliberate, general machinery (needed when both sides are genuinely
compound lambda terms) — not a bug by itself. It turns `matcher = X13` into
`X13 @ Y` (an applied free variable) and `to_match = t` into `t`'s body,
`ord_less_eq_a_a_o @ (λZ. ($@_var @ Y @ Z))` (`$@_var` is not a variable —
it's the display name, `cte_signature.c:258`, `SigInsertId(sig, "$@_var", 1,
true)`, for `SIG_PHONY_APP_CODE`, the generic wrapper `TermPrintDbgHO` uses
whenever a term's head is *any* applied variable, free or bound; the actual
identity — here another free variable from the tree's own indexed pattern
content — is the wrapper's first printed argument, not the label itself).

**Where the actual defect is.** On the next iteration `TermIsTopLevelFreeVar
(X13 @ Y)` is true, so `SubstComputeMatchHO` calls
`PartiallyMatchVar(X13, to_match, sig, false)` (`cte_match_mgu_1-1.c:132`),
confirmed via gdb to return `args_to_eat = 0` for this pair (`to_match =
ord_less_eq_a_a_o @ (λZ. ($@_var@db(1)@db(0)))`, arity 1). Right before
returning, `PartiallyMatchVar` is supposed to occurs-/closedness-check
whichever arguments it's about to fold into `X13`'s binding:
```c
for(int i=0; i<args_to_eat + TermIsAppliedAnyVar(to_match) ? 1 : 0; i++)
{
   if(!TermIsDBClosed(to_match->args[i]) || ...) { return MATCH_FAILED; }
}
```
(The `? 1 : 0` is redundant — `+` binds tighter than `?:`, so this is exactly
equivalent to `i < args_to_eat + TermIsAppliedAnyVar(to_match)` either way;
confusingly written, but not itself a bug.) With `args_to_eat=0` and
`to_match` headed by a real symbol (`TermIsAppliedAnyVar(to_match)=false`),
the loop bound is `0` — it **never runs**, so `to_match->args[0]` (the
lambda `λZ.($@_var@db(1)@db(0))`, which still references `db(1)`, the
*outer* bound variable `Y` that was just peeled off one level up) is never
checked for being `DB`-closed.

Back in `SubstComputeMatchHO`, `SubstBindAppVar(subst, X13, to_match,
args_eaten=0, bank)` is then called
(`TERMS/cte_subst.c:432`), which does:
```c
Term_p to_bind_pref = TermCreatePrefix(to_bind, up_to);  // up_to = 0
to_bind_pref->type = var->type;
var->binding = ...to_bind_pref...;
```
`TermCreatePrefix(to_match, 0)` (`cte_termfunc.c:3111`), since `to_match` is
not phony-app and `0 != ARG_NUM(to_match)`, takes its generic branch:
```c
int pref_len = arg_num + (TermIsPhonyApp(orig) ? 1 : 0);   // = 0
prefix = TermTopAlloc(orig->f_code, pref_len);              // fresh, 0-arity
for(int i=0; i<pref_len; i++) { prefix->args[i] = orig->args[i]; }  // copies nothing
```
i.e. it builds a **fresh, bare 0-arity term headed by `ord_less_eq_a_a_o`,
discarding `to_match`'s one actual argument** — `X13` ends up bound to bare
`ord_less_eq_a_a_o`, not to the term it was actually asked to match. This
isn't necessarily wrong in isolation — the design (confirmed by the
`assert(args_eaten + ARG_NUM(matcher) == ARG_NUM(to_match))` right after,
which holds: `0 + 1 == 1`) is that the discarded argument becomes a
*separate* job: `matcher`'s own leftover argument (`Y`, the fresh
placeholder `eta_expand_otf` introduced) gets pushed to be matched against
`to_match`'s leftover argument (the lambda). But `Y` is a bound-variable
placeholder, not a free variable — it can't meaningfully "become" or absorb
an arbitrary compound term the way a genuine free variable can, and nothing
downstream re-establishes the missing consistency check that would have
caught this. Confirmed via a full loop trace (breakpoint logging every
`matcher`/`to_match` pair) that this is exactly what happens: the two calls
after the `TermCreatePrefix` discard reduce, via a second
`PruneLambdaPrefix`/`eta_expand_otf` round on the leftover pair, down to a
final, trivially-successful reflexive comparison of a genuine bound
variable against itself (`db(1)` vs `db(1)`, confirmed via the `TPIsDBVar`
property flag, `cte_termtypes.h:93`) — which is *why* `res = MATCH_SUCC` is
reported despite the binding upstream having silently dropped real content.

**Corroboration from the λE paper** (`bugs/eprover-ho-paper.pdf`,
Vukmirović/Blanchette/Schulz, "Matching", p.121): *"We also modify the
algorithm to ensure that free variables are never bound to terms that have
loose bound variables."* That's precisely the invariant `PartiallyMatchVar`'s
skipped loop was meant to enforce — this is a documented design goal the
current code fails to uphold for this shape (pattern is a bare variable,
`args_to_eat` comes out to exactly `0`). The paper also documents a
fallback: `SubstMatchComplete` retries via a separate, more careful
`SubstComputeMatchPattern` when `SubstComputeMatchHO` returns failure *and*
both terms are in the "pattern" fragment (free variables applied only to
distinct de Bruijn indices — which `X13`/the target here both qualify as).
But that fallback is gated on an honest `res != MATCH_SUCC`; our bug's
failure mode is the opposite — a *false* success — so the more robust path
never gets a chance to run.

## Fix

**First attempt (insufficient).** Initially added a narrow guard in
`PartiallyMatchVar`, only for the exact `args_to_eat == 0` case traced above:
```c
if(args_to_eat == 0 && !TermIsAppliedAnyVar(to_match) && !TermIsDBClosed(to_match))
{
   return MATCH_FAILED;
}
```
This alone fixed `bug009`'s isolated reproducer, but re-running bug006's
*original* (unminimized) reproducer still crashed on the identical
assertion, just at a different occurrence. gdb showed the new occurrence has
`args_to_eat = 1` with `to_match->arity = 2` — i.e. the existing loop *does*
run this time (checks `to_match->args[0]`), but `to_match->args[1]` — a
genuine **leftover** argument beyond `args_to_eat` — is never checked at
all, in either the `args_to_eat == 0` or `args_to_eat > 0` case. The bug is
broader than the one instance first traced: leftover arguments (the ones
*not* absorbed into `var_matcher`'s binding, which become a separate
match job against `var_matcher`'s own applied arguments) were never
`DB`-closedness-checked, period.

**Actual fix** — generalized to cover every leftover argument, replacing the
narrow guard above (`TERMS/cte_match_mgu_1-1.c`, `PartiallyMatchVar`, right
after the existing `args_to_eat > ARG_NUM(to_match)` check):
```c
/* Arguments of to_match at indices >= args_to_eat (+1 if to_match's own
 * head is itself an applied variable) are not absorbed into
 * var_matcher's binding here -- they become a *separate* leftover
 * match against var_matcher's own applied arguments, pushed by the
 * caller (SubstComputeMatchHO). That leftover path assumes each
 * leftover argument is something a bound-variable placeholder can
 * meaningfully stand in for -- it isn't, if the argument references a
 * bound variable from the caller's local scope (not DB-closed).
 * Silently proceeding drops that content (see bug009). Fail honestly
 * here instead; SubstMatchComplete falls back to
 * SubstComputeMatchPattern for terms in the pattern fragment, which
 * handles this correctly. */
for(int i = args_to_eat + (TermIsAppliedAnyVar(to_match) ? 1 : 0); i < to_match->arity; i++)
{
   if(!TermIsDBClosed(to_match->args[i]))
   {
      return MATCH_FAILED;
   }
}
```
This subsumes the first attempt (the `args_to_eat == 0` case is just
`i` starting at `0`) without needing it as a separate branch.

**Third fix — a related but distinct gap, found while checking the above
against a release (`-O03`) build.** The `-O0` debug build's verification
above (clean on both bug006 scripts) turned out to be a **false negative**:
`-O0` is slow enough that it never reached deep enough into the search to
hit a further occurrence that the much-faster `-O03` release build finds
easily within the same 5-15s budget. Rerunning
`bug006-sup-partial-app-type-mismatch-original.sh` on a release build after
the two fixes above still crashed on the identical assertion. gdb (back on
an `-O0` rebuild, given a much larger CPU budget to reach the same search
depth) showed `s` — the top-level pattern in a `SubstComputeMatchHO` call
reached this time via **subsumption**
(`clause_subsumes_clause`/`eqn_list_rec_subsume`, `CLAUSES/ccl_subsumption.c`
— a third caller of `SubstMatchComplete`, alongside demodulation and
bug009's discrimination-tree matching), still contained a **bare** `$eq`,
despite the eta-reduction fix confirmed (via `-l2`) to correctly prevent
bare `$eq` during normal CNF for this exact problem.

The actual gap: both `SubstComputeMatchHO`'s and `SubstComputeMguHO`'s
rigid-rigid decomposition branches have a `SigIsPolymorphic` type-check —
but gated by `matcher->arity != 0` / `t1->arity != 0`, since a bare
(arity-0) occurrence has no `args[0]` to compare:
```c
if(matcher->f_code != to_match->f_code ||
   (!TermIsTopLevelDBVar(matcher)
     && SigIsPolymorphic(bank->sig, matcher->f_code)
     && matcher->arity != 0                                 // <-- skips the bare case entirely
     && matcher->args[0]->type != to_match->args[0]->type))
```
So two **bare** occurrences of the same polymorphic symbol (same `f_code`,
e.g. both literally `$eq`) at *different* types pass this check unchecked —
`matcher->f_code != to_match->f_code` is false (same symbol), and the
type-comparing conjunct is skipped outright since `arity == 0`. Fixed in
both locations (`TERMS/cte_match_mgu_1-1.c`) by comparing `matcher->type !=
to_match->type` directly when `arity == 0`, `args[0]->type` otherwise:
```c
if(matcher->f_code != to_match->f_code ||
   (!TermIsTopLevelDBVar(matcher)
     && SigIsPolymorphic(bank->sig, matcher->f_code)
     && ((matcher->arity != 0 && matcher->args[0]->type != to_match->args[0]->type) ||
         (matcher->arity == 0 && matcher->type != to_match->type))))
```
(and the structurally identical check in `SubstComputeMguHO`, same file).

## Fix verification (2026-09-15)

- `bugs/bug009-pdtree-ho-match-prefix-assertion.sh` (with `--cpu-limit=10`
  added, since the file has no conjecture): no crash, `SZS status
  ResourceOut`, on both `-O0` and `-O03` builds. Confirmed via gdb the
  leftover-argument guard actually fires during the run (178,584 times)
  rather than being coincidentally unreached.
- `bugs/bug006-sup-partial-app-type-mismatch.sh` (minimized): clean on both
  `-O0` and `-O03` builds after all three fixes.
- `bugs/bug006-sup-partial-app-type-mismatch-original.sh` (the original
  31-flag strategy, full unminimized problem): clean on `-O0`. **Still
  crashes on `-O03`** — a **fourth** occurrence of the same assertion (now
  at a different line, `cte_match_mgu_1-1.c:466`), not yet investigated.
  See "Not yet done".
- Regression pass (`EXAMPLE_PROBLEMS`, `bugs/bug001`–`bug005`/`bug007`/
  `bug008`): no regressions, both builds.

## Not yet done

- **The 4th occurrence found on the `-O03` release build is not fixed or
  investigated.** Reproduce with
  `bugs/bug006-sup-partial-app-type-mismatch-original.sh` built at `-O03`
  (release flags in `Makefile.vars`) — crashes within its own 5s/15s
  budget; the `-O0` debug build needs a much larger `--cpu-limit` to reach
  equivalent search depth (release is far faster) before the same gdb-based
  approach used for the first three occurrences will work.
- Given four occurrences of essentially the same "polymorphic symbol
  type-safety assumption breaks down for degenerate/bare cases" bug class
  found across three independent subsystems (eta-reduction, HO matching's
  leftover-argument handling, HO matching/unification's rigid-rigid
  decomposition) in one investigation, there may be a more fundamental,
  single root cause underlying all of them, or simply more scattered
  instances of the same class elsewhere in the matching/unification code —
  not established either way.
- Not established whether this is a genuinely pre-existing, latent bug that
  bug006's eta-reduction fix merely exposed (most likely, given that fix is
  narrow and doesn't touch matching/unification code at all), or whether
  it's a novel interaction the fix introduced. No evidence found for the
  latter, but not conclusively ruled out either.
- No fix has been reported upstream.
- `bugs/bug009-pdtree-ho-match-prefix-assertion.p` is not minimized beyond
  "all 111 demodulator clauses + target" — likely reducible much further,
  not attempted.
- Not run to completion with a large CPU budget — verification so far only
  confirms clean `ResourceOut` within short (5-25s) time limits, not that
  either strategy actually finds a proof of the underlying problem given
  more time.
