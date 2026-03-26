# Bug: Assertion `bank' failed in NormalizePatternAppVar

**Triggered by:** `ConjectureTermPrefixWeight` heuristic (`-H` argument)
**File:** `TERMS/cte_termbanks.c:2535`
**Fix:** `TERMS/cte_termfunc.c` — two one-line additions

**Reproduction:**
- `bugs/bug001-prefix-weight-null-owner-bank.sh` + `bugs/bug001-prefix-weight-null-owner-bank.p` — minimal (2-line problem)
- `bugs/ALG247^2.sh` + `bugs/ALG247^2.p` — original report
- `bugs/SYO548^1.sh` + `bugs/SYO548^1.p` — second original report

---

## Symptom

All three reproduce the same crash during heuristic initialization, before any inferences:

```
eprover-ho: cte_termbanks.c:2535: NormalizePatternAppVar: Assertion `bank' failed.
```

---

## Minimal problem

```tptp
thf(conj, conjecture,
    ? [Q: $i > $o, X: $i] : ( Q @ X )).
```

After clausification, `Q @ X` appears as a literal with `Q` a free variable of type
`$i > $o` — an *applied free variable*. This is the construct that triggers the crash.

---

## Root Cause

`NormalizePatternAppVar(TB_p bank, Term_p s)` asserts `bank` is non-NULL; it needs the
bank to call `LambdaEtaReduceDB`. The caller obtains it via `TermGetBank(term)`, which
reads `term->owner_bank`.

`TermTopCopy` → `TermTopCopyWithoutArgs` **unconditionally zeroes `owner_bank`**
(`TermSetBank(handle, 0)` at `cte_termtypes.h:581`). Two term-copying functions use
`TermTopCopy` without restoring the bank:

- **`TermCopyUnifyVars`** — zeroes `owner_bank` at every node of the copy tree. The
  applied free variable appears at the **root** of the copy, so `TermGetBank(root) == NULL`
  immediately triggers the assertion. (ALG247^2 and minimal repro path.)

- **`TermCopyRenameVars`** — same issue, but the applied free variable can appear at an
  **interior node** (e.g. under a lambda: `λ(Q @ X)`). Even if the root's bank is
  patched elsewhere, interior nodes still have `owner_bank == NULL`. (SYO548^1 path.)

`TermCopy` (`cte_termfunc.c:1362`) already propagates the bank correctly with
`TermSetBank(handle, TermGetBank(source))`; both buggy functions are newer additions that
did not carry this forward.

### Why HO-only

`NormalizePatternAppVar` is compiled only under `ENABLE_LFHO`, and `TermWeightCompute`
only reaches it for applied free variables, which do not exist in first-order problems.

### Call chain

```
main
└─ ProofStateInit                         cco_proofproc.c:1499
   └─ HCBClauseEvaluate                   che_hcb.c:885
      └─ ClauseAddEvaluation              che_wfcb.c:109
         └─ ConjectureTermPrefixWeightCompute  che_prefixweight.c:405
            └─ prfx_init  (lazy init)     che_prefixweight.c:196
               └─ prfx_insert_subgens     che_prefixweight.c:140
                  └─ prfx_insert_term     che_prefixweight.c:74
                     │  norm = TermCopyNormalizeVars(...)
                     │  // norm->owner_bank == NULL  ← bug
                     └─ PDTreeInsertTerm  ccl_pdtrees.c:1125
                        └─ TermStandardWeight(norm)
                           └─ TermWeightCompute      cte_termfunc.c:1879
                              │  TermIsAppliedFreeVar(norm) → true
                              └─ NormalizePatternAppVar(TermGetBank(norm), norm)
                                 // TermGetBank(norm) == NULL → assert fires
```

---

## Fix

**`TERMS/cte_termfunc.c`** — two one-line additions:

In `TermCopyRenameVars`, propagate the bank at each recursive node:

```c
 copy = TermTopCopy(term);
 copy->type = term->type;
+TermSetBank(copy, TermGetBank(term));
 for (i=0; i<term->arity; i++)
     copy->args[i] = TermCopyRenameVars(renaming, term->args[i]);
```

In `TermCopyUnifyVars`, propagate the bank at each recursive node:

```c
 Term_p new = TermTopCopy(term);
 for (i=0; i<term->arity; i++)
 {
     new->args[i] = TermCopyUnifyVars(vars, term->args[i]);
 }
+TermSetBank(new, TermGetBank(term));
 return new;
```

Both are symmetric with `TermCopy`'s existing behaviour.

---

## Verification

After the fix, all three scripts complete without assertion failure:

```sh
bash bugs/bug001-prefix-weight-null-owner-bank.sh   # → % SZS status Theorem (trivial)
bash bugs/ALG247^2.sh                        # → % SZS status GaveUp
bash bugs/SYO548^1.sh                        # → % SZS status GaveUp
```

(`GaveUp` is expected for ALG247^2 and SYO548^1 — the one-second soft CPU limit is too
short to find a proof.)
