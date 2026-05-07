# TODO — HO ENIGMA Feature Encoding / ProofLog

---

## Issue 1 — ProofLog: saturation trace for explosive-clause detection

Add `--proof-log <file>` to `eprover` / `eprover-ho`. Overwrites file each run.
Goal: collect per-given-clause blast data to train a model filtering explosive clauses.

### Log format

One block per GIVEN step, separated by blank lines. Formula printed **only** on the
GIVEN line (when the clause is first selected); GEN/BW lines use id+lits only.
FW-deleted clauses (never become GIVEN) also get their formula since it won't appear
elsewhere.

```
GIVEN 42 gen=4 bw=2 fw=1 lits=2: f(X,Y) | g(Z)
  + GEN 43 lits=1
  + GEN 44 lits=2
  + GEN 45 lits=1
  + GEN 46 lits=1
  - BW_SUB 37
  - BW_RW 38
  ! FW 99 lits=2: redundant_clause(X)
GIVEN 43 gen=1 bw=0 fw=0 lits=1: a = b
  + GEN 91 lits=3
PROOF 12 43 91
```

Line prefixes: `+` generated, `-` backward deleted, `!` forward deleted before
entering unprocessed. IDs are `perm_ident` (stable; never mutated by logging calls,
unlike `ident`).

### Subtasks

- [x] Add `OPT_PROOF_LOG` to `PROVER/e_options.h` and wire up in
      `PROVER/eprover.c:process_options`; open file in append mode, store as
      `FILE *ProofLog` global (analogous to `GlobalOut`)
- [x] In `ProcessClause` (`CONTROL/cco_proofproc.c`): buffer the current GIVEN step
      in a small local struct (given perm_id, formula, list of GEN/BW/FW entries);
      flush as a formatted block after `insert_new_clauses` returns
- [x] After `generate_new_clauses`: walk `state->tmp_store` and record each clause
      as a GEN entry (perm_id, lits); clauses surviving `insert_new_clauses` remain
      GEN; those killed become FW entries
- [x] Scan `tmp_store` after all BW eliminations (before `ClauseSetSetProp`) to
      record BW entries; all BW types collapsed to `BW` for now
- [x] After proof found: walk derivation chain from empty clause, collect ancestor
      perm_ids, emit `PROOF <id1> <id2> ...` line
- [ ] Add `--no-proof-log-formulas` flag to suppress formula text (ids+lits only)
      for smaller logs

### Python post-processor (separate script)

- [x] `scripts/proof_log_analysis.py`: parse log, build given→children graph
- [x] Compute per-given: direct_gen, bw_deleted, fw_deleted, net_added_to_unprocessed
- [x] Compute transitive blast radius (follow GEN→GIVEN chain)
- [x] Mark proof-relevant given clauses (ancestors of PROOF entries)
- [x] Output CSV: `block, perm_id, lits, direct_gen, bw, fw, net, blast_radius, is_proof_relevant`

---

## Issue 3 — DB variable index not encoded (Design choice)

All DB vars map to `"^"` (or `"^:type"` in typed mode) regardless of their index.
This means `λx.λy. x` and `λx.λy. y` produce identical features for the bound vars —
depth/scope within nested lambdas is lost. Analogous to how all free vars become `*`.

Probably intentional (scope-independence), but worth reconsidering for HO problems
where lambda depth matters. Could be added as an option (e.g. `v[d]` to encode DB index).

---

## Non-issue — Lambda-headed phony apps

`(λ body) @ a` (unreduced beta-redex) is encoded as any other phony app. These should
not appear in clauses after `BetaNormalizeDB` runs during clausification. Not a
practical concern.

---

## Summary Table

| # | Issue | Severity | File | Status |
|---|-------|----------|------|--------|
| 1 | ProofLog: saturation trace | High | `cco_proofproc.c`, `eprover.c` | Open |
| 3 | DB var index not encoded | Design choice | `symbol_string`, `FCODE` | Deferred |
| — | Lambda-headed phony apps | Non-issue | — | N/A |
| ~~1~~ | ~~Head invisible in vertical walks~~ | High | `update_term`, `update_verts` | Fixed |
| ~~2~~ | ~~Head as sibling in horizontal walks~~ | Medium | `update_horiz` | Fixed |
| ~~4~~ | ~~DB lambda placeholder traversed in untyped mode~~ | Low | `update_term` | Fixed |

---

## Completed

### ~~Issue 1 — Head of phony app invisible in vertical walks~~ (Fixed)

`update_term` recurses into all args uniformly with `term->arity`, so for phony app
`X @ a @ b` (f_code=17, args=[X, a, b]) the path at leaf `a` is:

```
[+, $@_var, a]    →  vertical walk: +|$@_var|a
```

whereas FO `f(a, b)` gives:

```
[+, f, a]         →  vertical walk: +|f|a
```

In FO the walk captures which function `a` appears under. In HO the head `X` is
invisible — `X @ a @ b` and `Y @ a @ b` produce identical vertical walks at `a`.
The head IS present in the horizontal walk of the phony app node itself
(`. $@_var . X . a . b .`), but only as a flat sibling of a and b.

**Fix**: in `update_term`, detect phony apps and push `args[0]` (the head) onto the
path instead of (or in place of) the phony app node, then iterate over `args[1..n]`.
This makes the vertical walk at `a` see the actual head symbol as parent, mirroring FO.

Note: `ARG_NUM` macro is already defined in `cte_termtypes.h:204`:
```c
#define ARG_NUM(term) (TermIsPhonyApp(term) ? (term)->arity-1 : (term)->arity)
```
It was never used in `che_enigmaticvectors.c` — its existence marks that the
head/args asymmetry was recognized but not yet addressed.

---

### ~~Issue 2 — Head treated as sibling in horizontal walks~~ (Fixed)

`update_horiz` for phony app `X @ a1 @ a2` produces:

```
. $@_var . X_sym . a1_sym . a2_sym .
```

But FO `f(a1, a2)` produces:

```
. f . a1_sym . a2_sym .
```

In FO the function is the root; in HO `$@_var` is the root and the head `X` is listed
as a peer of the actual arguments. The model cannot distinguish `X @ a1 @ a2` from
`Y @ a1 @ a2` by the horizontal walk structure alone.

**Fix**: when `term` is a phony app, use `args[0]` as the root symbol of the horizontal
walk and iterate over `args[1..n]` as children:

```c
// current:  . $@_var . X . a1 . a2 .
// fixed:    . X . a1 . a2 .
```

---

### ~~Issue 4 — DB lambda placeholder visited in untyped mode~~ (Fixed)

`$db_lam(^, body)`: `args[0]` is the placeholder DB var (carries type, no semantic
content). `update_term` visits it, generating path features at `[..., $db_lam, ^]`.

- **Untyped mode**: all placeholders produce identical `^` — uniform noise.
- **Typed mode**: `symbol_string` returns `"^:type"`, encoding the bound variable's
  type — this is actually useful.

**Fix**: in `update_term`, skip `args[0]` of a DB lambda in untyped mode (only recurse
into `args[1]` = body). Typed mode is unchanged.
