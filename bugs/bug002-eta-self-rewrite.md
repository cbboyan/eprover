# Bug: Assertion `term!=replace' failed in TermAddRWLink

**File:** `CLAUSES/cte_replace.c:71`
**Fix:** `CLAUSES/ccl_rewrite.c` — six-line guard

**Reproduction:**
- `bugs/bug002-eta-self-rewrite.sh` + `bugs/bug002-eta-self-rewrite.p` — minimal (7-axiom problem)
- `bugs/ITP035^1.sh` + `bugs/ITP035^1.p` — original report

---

## Symptom

```
eprover-ho: cte_replace.c:71: TermAddRWLink: Assertion `term!=replace' failed.
```

---

## Minimal problem

```tptp
thf(comp_apply, axiom,
    ( comp_c130555887omplex
    = ( ^ [F2: complex > complex, G: complex > complex, X: complex] : ( F2 @ ( G @ X ) ) ) ) ).
thf(comp_id, axiom, ! [F: complex > complex] : ( comp_c130555887omplex @ F @ id_complex ) = F ).
thf(id_apply, axiom, ( id_complex = ( ^ [X: complex] : X ) ) ).
thf(deriv_id, axiom, ( deriv_complex @ id_complex ) = ( ^ [Z2: complex] : one_one_complex ) ).
thf(conj_0, conjecture, ( deriv_complex @ id_complex @ w ) = one_one_complex ).
```

(Plus type declarations for `complex`, `comp_c130555887omplex`, `id_complex`,
`deriv_complex`, `one_one_complex`, `w`.)

### Derivation trace (from `-l4` output)

```
c_0_21:  comp(X1, X2, X3) = X1 @ (X2 @ X3)             [comp_apply, unfolded]
c_0_25:  id(X3) = X3                                     [id_apply, unfolded]
c_0_23:  comp(X1, id) = X1                               [comp_id]
c_0_26:  comp(X1, ^[Z]:Z) = X1                           [c_0_23 rw by c_0_25: id→^[Z]:Z]
c_0_27:  ![X1]: ^[Z]: X1 @ ((^[Y]:Y) @ Z) = X1          [c_0_26 rw by c_0_21: comp unfold]
c_0_28:  ![X1]: ^[Z]: X1 @ Z = X1                        [c_0_27 beta: (^[Y]:Y)@Z → Z]
                                              ↑ eta demodulator now active

... (later, from deriv_id + id_apply) ...

         deriv(^[Z]:Z) @ X = 1    ← contains ^[Z]:Z as subterm

ForwardContractClause applies c_0_28 to subterm ^[Z]:Z:
  match  ^[Z]: X1 @ Z  against  ^[Z]: Z
  HO:    X1 @ Z = Z  →  X1 ↦ ^[Z]:Z  (identity)
  repl = X1 = ^[Z]:Z = term          ← repl == term → CRASH
```

### Why composition axioms are needed

The 3 proof axioms alone (`id_apply`, `deriv_id`, `conj_0`) find the proof immediately in
2 steps and never derive the eta demodulator. The composition axioms (`comp_apply` +
`comp_id`) are not in the proof, but they trigger the dangerous derivation chain:

1. `comp(F, id) = F` + `id → ^[X]:X` → `comp(F, ^[X]:X) = F`
2. `comp_apply` unfolds LHS: `^[Z]: F @ ((^[X]:X) @ Z) = F`
3. Beta-reduces to **`^[Z]: F @ Z = F`** — the eta demodulator

Only then does the `deriv` clause (which rewrites `id → ^[Z]:Z`) have a live demodulator
waiting to fire on `^[Z]:Z`.

---

## Root Cause

The demodulator `![F: T>T]: (^[Z:T]: F @ Z) = F` (an eta-reduction rule) is matched
against the identity term `^[Z:T]: Z` via HO matching:

- LHS `^[Z]: F @ Z` matched against `^[Z]: Z`
- Bodies: `F @ Z` vs `Z` — solved by `F ↦ ^[Z]:Z` (identity)
- Instantiated RHS: `F` = `^[Z]:Z` = the **original term**
- `repl == term` triggers `TermAddRWLink`'s assertion `term != replace`

The ordering check (`instance_is_rule`) passes because it dereferences `F` only once
(seeing `^[Z]:F(Z)` > `F`), but `MakeRewrittenTerm` then beta/eta-normalizes the RHS,
collapsing it back to the LHS instance.

### Why HO-only

In FO mode `MakeRewrittenTerm` is a no-op and the RHS is never lambda-normalized, so it
cannot collapse back to the LHS. In HO mode the `LambdaNormalizeDB` call inside
`MakeRewrittenTerm` can produce this collapse.

### Call chain

```
main
└─ Saturate                          cco_proofproc.c:1763
   └─ ProcessClause                  cco_proofproc.c:1595
      └─ ForwardContractClause       cco_forward_contraction.c:368
         └─ ForwardModifyClause      cco_forward_contraction.c:266
            └─ ClauseComputeLINormalform  ccl_rewrite.c:1261
               └─ eqn_li_normalform  ccl_rewrite.c:897
                  └─ term_li_normalform   ccl_rewrite.c:842
                     └─ term_subterm_rewrite  ccl_rewrite.c:773
                        └─ term_li_normalform   ccl_rewrite.c:832
                           └─ rewrite_with_clause_set  ccl_rewrite.c:680
                              └─ TermAddRWLink(term, repl, ...)
                                 // term == repl → assert fires
```

---

## Fix

**`CLAUSES/ccl_rewrite.c`** — skip the rewrite if the result equals the original term:

```c
 repl = TBInsertInstantiated(bank, ClausePosGetOtherSide(pos));
 if(problemType == PROBLEM_HO)
 {
     repl = MakeRewrittenTerm(term, repl, 0, bank);
 }
+if(repl == term)   /* trivial HO rewrite — skip */
+{
+    SubstDelete(subst);
+    return term;
+}
 assert(pos->clause->ident);
 TermAddRWLink(term, repl, ...);
```

Safe: returning `term` unchanged still updates the NF date in `term_li_normalform`,
preventing the same demodulator from re-firing. The `while(modified)` loop terminates.

---

## Verification

```sh
bash bugs/bug002-eta-self-rewrite.sh   # → SZS status Theorem
bash bugs/ITP035^1.sh                  # → SZS status Theorem
```
