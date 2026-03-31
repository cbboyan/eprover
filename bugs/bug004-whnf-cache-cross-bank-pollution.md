# Bug: WHNF Cache Cross-Bank Pollution (use-after-free)

**File:** `bugs/bug004-whnf-cache-cross-bank-pollution.p` (copy of `bugs/0112_Transcendental_01863.p` — not minimized)
**Trigger:** `bash bugs/bug004-whnf-cache-cross-bank-pollution.sh`
**Status:** Fixed.

---

## Symptom

Non-deterministic crash during HO preprocessing. The same run sometimes
finishes (or crashes elsewhere), sometimes asserts:

```
eprover-ho: cte_lambda.c:673: do_beta_normalize_db: Assertion `TermIsShared(res)' failed.
```

With debug instrumentation in `do_beta_normalize_db`:

```
do_beta_normalize_db: unshared result from branch 'whnf'
  t   (f_code=17 arity=2): $@_var @ ($db_lam @ db(0) @ ($db_lam @ db(0) @ ($@_var @ X206 @ db(0) @ db(1)))) @ db(0)
  res (f_code=-9223372036854774387 arity=0): Y4611686018427387194
```

The corrupted `res` has `arity=0` and huge-negative `f_code` — consistent with
the WHNF cache returning a pointer to memory that was freed and reused.

This is a **cache hit** path: the `assert(TermIsShared(res))` inside `whnf_step_uncached`
does not fire, confirming the fresh computation is valid. The corruption is in the
CACHED value, i.e., `t->binding_cache` points to a freed/reused term cell.

Roughly 50% of runs fail — non-deterministic due to heap layout (ASLR).

---

## Root Cause: TBInsertNoProps "Cheat" Causes Cross-Bank WHNF Cache Pollution

### Call chain to the bug

```
ProofStateClausalPreproc
  ClauseSetUnfoldEqDefNormalize(tmp_terms=0x7ffff7c35010, ...)
    ClauseSetFilterTautologies(work_bank=tmp_terms)
      ClauseIsTautologyReal(work_bank=tmp_terms)
        ClauseCopy(clause, bank=tmp_terms)
          EqnCopy(bank=tmp_terms)
            TBInsertNoPropsCached(bank=tmp_terms, term=state_phony_app)
              TBInsertNoProps(bank=tmp_terms, term=state_phony_app)
```

### The "Cheat" in TBInsertNoProps

`TBInsertNoProps` in `TERMS/cte_termbanks.c` (lines 1009–1019) temporarily changes
the `owner_bank` of the input term to the destination bank before calling `WHNF_deref`:

```c
TB_p tmp_bank = TermGetBank(term);               // = state->terms
TermSetBank(term, bank);  //Cheat because WHNF_deref() needs it!
t = WHNF_deref(term);
TermSetBank(term, tmp_bank);                     // restores state->terms
term = t;
```

When `bank = tmp_terms` and `term` is a `state->terms` lambda-headed phony app:

1. `TermSetBank(term, tmp_terms)` — `term->owner_bank` is temporarily `tmp_terms`
2. `WHNF_deref(term)` → `WHNF_step(TermGetBank(term)=tmp_terms, term)` → cache miss
   → `whnf_step_uncached(tmp_terms, term)` → allocates `res` in `tmp_terms`
   → **`TermSetCache(term, res)`** — `term->binding_cache = res` (a `tmp_terms` term)
3. `TermSetBank(term, state->terms)` — restores `owner_bank`, but **`binding_cache` is
   now a cross-bank pointer**: a `state->terms` term caching a `tmp_terms` result.

### The use-after-free

`TBGCSweep(tmp_terms)` is called at the end of each saturation cycle. It sweeps
`tmp_terms` with only `true_term/false_term/min_terms` as roots — it is a full
collection of everything else in `tmp_terms`. This frees `res`.

Now `state_phony_app->binding_cache` is dangling.

The next call to `WHNF_step(state->terms, state_phony_app)` returns the dangling
pointer as a cache hit. `do_beta_normalize_db` then calls `TermIsShared(res)` on
freed memory → assertion crash.

### GDB confirmation

GDB watchpoint on `state_phony_app->binding_cache` (after `ASLR` disabled):

```
Hardware watchpoint 3: ((struct termcell*)t)->binding_cache
Old value = (struct termcell *) 0x0
New value = (struct termcell *) 0xcb0550
#0  whnf_step_uncached (bank=0x7ffff7c35010, t=0xc8ad00) at TERMS/cte_lambda.c:1191
#3  TBInsertNoProps (bank=bank@entry=0x7ffff7c35010, term=0xc8ad00, ...) at TERMS/cte_termbanks.c:1017
#11 ClauseIsTautologyReal (work_bank=work_bank@entry=0x7ffff7c35010, ...) at CLAUSES/ccl_tautologies.c:371
#13 ClauseSetUnfoldEqDefNormalize (..., tmp_terms=0x7ffff7c35010, ...) at CLAUSES/ccl_unfold_defs.c:410
```

`bank=0x7ffff7c35010` is `tmp_terms`. The cache was set on a `state->terms` term
during a `tmp_terms` WHNF computation triggered by the cheat.

---

## Fix

In `TBInsertNoProps` (`TERMS/cte_termbanks.c`), after restoring the original bank,
clear the WHNF cache if cross-bank pollution occurred:

```c
TermSetBank(term, tmp_bank);
#ifdef ENABLE_LFHO
// Clear WHNF cache if WHNF_deref set it using the temporary bank (cross-bank
// pollution). The term lives in tmp_bank but the cached result is allocated in
// bank. If bank is GC-swept independently (e.g. tmp_terms sweep), the cache
// pointer becomes dangling and causes a use-after-free.
if(bank != tmp_bank)
{
   TermSetCache(term, NULL);
}
#endif
term = t;
```

### Secondary fix (belt-and-suspenders)

`TBGCMarkTerm` in `TERMS/cte_termbanks.c` previously only followed `binding_cache`
for applied-free-var phony apps:

```c
if(TermIsAppliedFreeVar(term) && TermGetCache(term))  // OLD
```

The `TermIsAppliedFreeVar` guard was wrong — WHNF caches are on **lambda-headed**
phony apps, not applied-free-var ones. Changed to:

```c
if(TermGetCache(term))  // NEW
```

This ensures the GC marks WHNF-cached results from the main bank during
`TBGCCollect(state->terms)`, providing belt-and-suspenders protection even if a
cross-bank cache somehow persists.

---

## Verification

Run `bash bugs/0112_Transcendental_01863.sh` at least 10 times — should always
complete with `% SZS status` output and no assertion failure.

---

## Files Changed

- `TERMS/cte_termbanks.c` — primary fix in `TBInsertNoProps`; secondary fix in `TBGCMarkTerm`
