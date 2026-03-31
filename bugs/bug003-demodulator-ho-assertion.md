# Bug: ccl_rewrite.c:632: indexed_find_demodulator: Assertion `false' failed.

**Triggered by:** Demodulator rewriting in HO mode (default auto-schedule)

**File:** `CLAUSES/ccl_rewrite.c:632`

**Fix:** `CLAUSES/ccl_rewrite.c` — guard debug assertion with `problemType != PROBLEM_HO`

**Reproduction:**
- `bugs/bug003-demodulator-ho-assertion.sh` + `bugs/bug003-demodulator-ho-assertion.p` — minimal (4 formulae, synthesized)
- `bugs/ITP109^1.sh` + `bugs/ITP109^1.p` — original report

---

## Symptom

```
eprover: ccl_rewrite.c:632: indexed_find_demodulator: Assertion `false' failed.
```

Debug output printed before the crash:

```
Term groups1862963056a_real @ ($db_lam @ db(0) @ (groups1862963056a_real @ ($db_lam @ db(0) @
  ($@_var @ X447 @ db(0) @ db(1))) @ X55)) @ X126
derefed { groups1862963056a_real @ ($db_lam @ db(0) @ (groups1862963056a_real @ ($db_lam @ db(0) @
  ($@_var @ ($db_lam @ db(0) @ ($db_lam @ db(0) @ ($@_var @ X57 @ db(1)))) @ db(0) @ db(1))) @ X55)) @ X126 }
should match
  groups1862963056a_real @ ($db_lam @ db(0) @ (groups1862963056a_real @ X57 @ X55)) @ X126,
substitution is : {X447<-^[Z0:a, Z1:a]:(X57 @ Z0), X55<-X55, X126<-X126}.
```

---

## Root Cause

The assertion at `ccl_rewrite.c:617–633` is a debug sanity check verifying that the
demodulator selected by `PDTreeFindNextDemodulator` structurally matches the target term:

```c
#ifndef NDEBUG
   if(res
      && !TermStructPrefixEqual(ClausePosGetSide(res), term, DEREF_ONCE, DEREF_NEVER, 0, ocb->sig))
   {
      ...
      assert(false);
   }
#endif
```

`TermStructPrefixEqual` with `DEREF_ONCE`/`DEREF_NEVER` dereferences the demodulator LHS
once, applying variable bindings, but does **not** perform full beta-normalization on the
resulting term. In HO mode, a valid demodulator match can produce a substitution where the
LHS, after applying the unifier, still contains unreduced lambda redexes. These redexes
reduce (via beta-normalization) to a term that matches the target, but the structural
comparison sees the unreduced form and incorrectly reports a mismatch.

In the failing case:
- Substitution: `X447 ← ^[Z0:a, Z1:a]:(X57 @ Z0)`
- Demodulator LHS after DEREF_ONCE: has `$@_var @ X447 @ db(0) @ db(1)` with X447 substituted
  to a lambda — the application is a beta redex
- After full beta-normalization: reduces to `X57 @ db(0)`, matching the target term
- The check uses incomplete comparison and fires

The actual rewriting code (`MakeRewrittenTerm`, guarded by `if(problemType == PROBLEM_HO)`)
does apply lambda normalization and produces the correct rewritten term. Only the debug
assertion is wrong.

**Note:** `NDEBUG` is not defined in normal builds (`Makefile.vars` line 146 has
`NODEBUG = #-DNDEBUG`), so this assertion fires in production.

### Why HO-only

In FO mode there are no lambda abstractions, so DEREF_ONCE is always sufficient to reduce
a substituted term to its final structural form. Beta-normalization is a HO-only concern.

### Call chain

```
rewrite_with_clause_set()          ccl_rewrite.c:652
  → indexed_find_demodulator()     ccl_rewrite.c:666
      → PDTreeFindNextDemodulator()    [finds valid HO match]
      → debug assertion check          ccl_rewrite.c:617  ← CRASH
          TermStructPrefixEqual(lhs, term, DEREF_ONCE, DEREF_NEVER, 0, sig)
              → does not beta-normalize substituted term
              → structural mismatch → assert(false)
```

---

## Minimal problem

2 type declarations + 2 axioms = 4 formulae (no conjecture needed):

```tptp
thf(sy_sum,type, sum: ( a > a ) > set_a > a).
thf(sy_op,type, op: a > a > a).

thf(ax1,axiom,
    ! [F: a > a,G: a > a,A: set_a] :
      ( ( sum @ ^ [X: a] : ( op @ ( F @ X ) @ ( G @ X ) ) @ A )
      = ( op @ ( sum @ F @ A ) @ ( sum @ G @ A ) ) ) ).

thf(ax2,axiom,
    ! [A: set_a,G: a > a > a] :
      ( ( sum @ ^ [X: a] : ( sum @ ( G @ X ) @ A ) @ A )
      = ( sum @ ^ [Y: a] : ( sum @ ^ [X: a] : ( G @ X @ Y ) @ A ) @ A ) ) ).
```

`ax1` is the demodulator (sum distributes over `op`). `ax2` is the Fubini/swap
identity for double sums. Together, E applies the `ax1` demodulator to the subterm
`sum(^[Y]. sum(G(Y), A), A)` from `ax2`, triggering the HO unification that creates
the beta redex.

### Derivation trace (from `-l4` output)

The crash occurs during the saturation loop initialization. `ax1` is immediately
oriented as a demodulator (LHS larger than RHS in the ordering). E then tries to
apply it to the subterm `sum(^[Y:a]. sum(G(Y), A), A)` from `ax2`, matching the
pattern `sum(^[X]. op(F(X), G_dmod(X)), A)` against the target.

The HO unifier (flex-rigid) binds the demodulator variable `F` or `G_dmod` (type
`a -> a`) to a lambda abstraction such as `^[Z:a]. sum(G(Z), A)`. After substitution
into the demodulator LHS, the applied variable (`F @ db(0)` or `G_dmod @ db(0)`)
becomes a beta redex `(^[Z:a]. sum(G(Z), A)) @ db(0)` that reduces to
`sum(G(db(0)), A)` — but the debug check (`TermStructPrefixEqual` with `DEREF_ONCE`)
sees the unreduced form and fires `assert(false)`.

### Why no conjecture is needed

The crash occurs purely from processing the axioms, without a goal clause. E
immediately applies the `ax1` demodulator to the clausified `ax2` during the
saturation initialization phase. A `$false` conjecture (or any goal) would also
work but is not required.

---

## Fix

In `CLAUSES/ccl_rewrite.c`, line 618, add `problemType != PROBLEM_HO`:

```c
#ifndef NDEBUG
   if(res
      && problemType != PROBLEM_HO
      && !TermStructPrefixEqual(ClausePosGetSide(res), term, DEREF_ONCE, DEREF_NEVER,
                                0, ocb->sig))
   {
      fprintf(stderr, "Term ");
      TermPrintDbg(stderr, ClausePosGetSide(res), ocb->sig, DEREF_NEVER);
      fprintf(stderr, " derefed { ");
      TermPrintDbg(stderr, ClausePosGetSide(res), ocb->sig, DEREF_ONCE);
      fprintf(stderr, " } should match ");
      TermPrintDbg(stderr, term, ocb->sig, DEREF_NEVER);
      fprintf(stderr, ", substitution is : ");
      SubstPrint(stderr, subst, ocb->sig, DEREF_NEVER);
      fprintf(stderr, ".\n");

      assert(false);
   }
#endif
```

This follows the existing `problemType != PROBLEM_HO` / `problemType == PROBLEM_HO`
runtime check pattern already used throughout `ccl_rewrite.c` (lines 209, 227, 258, 674).

---

## Verification

```sh
# Rebuild with fix applied
./configure --enable-ho && make -j20 rebuild

# Minimal reproducer — should not crash:
cd bugs && bash bug003-demodulator-ho-assertion.sh

# Original problem — should not crash:
bash ITP109^1.sh

# FO smoke test — assertion still fires for FO bugs:
eprover BOO001-1+rm_eq_rstfp.lop
```
