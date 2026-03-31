# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## About This Fork

This is a fork of [E Prover](http://www.eprover.org) (version 3.2 "Singbulli"), an equational theorem prover for first-order and higher-order logic. The `master` branch focuses on bug fixes (primarily in higher-order reasoning). The `enigma` branch maintains an ML-based clause selection guidance system — do not touch that branch unless explicitly asked.

The user ("yan") co-authors heuristic weight functions; files with `yan` in author comments are in `HEURISTICS/` (e.g., `che_levweight.c`, `che_prefixweight.c`, `che_strucweight.c`, `che_termweight.c`, `che_termweights.c`, `che_tfidfweight.c`, `che_treeweight.c`).

## Build

```sh
# First-order only (in-place build):
./configure
make -j$(nproc) rebuild

# Higher-order enabled (needed to run/test HO bugs):
./configure --enable-ho
make -j$(nproc) rebuild

# After config change, always use rebuild (not plain make):
make -j$(nproc) rebuild
```

Binaries land in `PROVER/`:
- `PROVER/eprover` — first-order only
- `PROVER/eprover-ho` — higher-order enabled

The HO flag is `-DENABLE_LFHO`. HO-specific code is wrapped in `#ifdef ENABLE_LFHO` or `#ifdef HAVE_LFHO` guards throughout the codebase.

## Running and Testing

```sh
# Basic smoke test (first-order):
cd PROVER && ./eprover BOO001-1+rm_eq_rstfp.lop

# Recommended invocation:
./eprover --auto --proof-object problem.p
./eprover-ho --auto-schedule --proof-object problem.p

# Reproduce a known bug:
bash bugs/ALG247^2.sh
bash bugs/ITP035^1.sh
bash bugs/SYO548^1.sh

# Run on an HO problem:
cd PROVER && ./eprover-ho thf/some_problem.p
```

There is no automated test suite invocation (no `make test`). Testing is done by running the prover on specific problem files and checking for crashes or wrong output.

## Bug Tracking

Bugs are documented in `bugs/` using a numbered scheme:
- `bugNNN-<description>.md` — root cause, call chain, fix, verification
- `bugNNN-<description>.p` — minimal TPTP reproduction case
- `bugNNN-<description>.sh` — minimal eprover invocation to reproduce
- Original TPTP problem `.sh` files (e.g. `ALG247^2.sh`) are kept as-is and reference the `.md`

When investigating a new bug: minimize the eprover args in `.sh` (usually just `-H'(...)'`),
minimize the `.p` to the smallest TPTP problem that triggers it, then write the `.md`.

Current open bugs: none

Fixed bugs:
- `bug001-prefix-weight-null-owner-bank` — assertion `bank` failed in `NormalizePatternAppVar` (triggered by `ALG247^2` and `SYO548^1`; fix: `TermSetBank` in `TermCopyRenameVars` and `TermCopyUnifyVars` in `TERMS/cte_termfunc.c`)
- `bug002-eta-self-rewrite` — assertion `term!=replace` in `TermAddRWLink` (HO self-rewrite: `MakeRewrittenTerm` normalizes RHS back to original term; fix: guard all 4 `TermAddRWLink` call sites in `CLAUSES/ccl_rewrite.c`; triggered by `ITP035^1` and `ITP137^1 --prefer-initial-clauses`)
- `ITP109^1` — assertion `false` in `indexed_find_demodulator` (HO demodulator match requires beta-normalization that debug check skips; fix: guard with `problemType != PROBLEM_HO` in `CLAUSES/ccl_rewrite.c`; minimal: `bugs/bug003-demodulator-ho-assertion.p` — 4 formulae, synthesized directly)
- `bug004-whnf-cache-cross-bank-pollution` — assertion `TermIsShared(res)` in `do_beta_normalize_db` (WHNF cache cross-bank pollution: `TBInsertNoProps` cheat temporarily sets `term->owner_bank=tmp_terms`, causing `WHNF_deref` to cache a `tmp_terms` result on a `state->terms` term; `TBGCSweep(tmp_terms)` then frees the cached result, leaving a dangling `binding_cache`; fix: clear `TermSetCache(term, NULL)` in `TBInsertNoProps` when `bank != tmp_bank`; secondary fix: broaden `TBGCMarkTerm` to follow `binding_cache` on all terms, not just applied-free-vars; in `TERMS/cte_termbanks.c`)

## Code Architecture

The codebase is structured as a set of C libraries compiled in dependency order, linked into final executables in `PROVER/`. Module names use prefixes:

| Directory | Prefix | Purpose |
|-----------|--------|---------|
| `BASICS/` | `clb_` | Memory, strings, generic data structures |
| `INOUT/` | `cio_` | Parsing (LOP, TPTP-2, TPTP-3), I/O |
| `TERMS/` | `cte_` | Term representation, unification, lambda calculus |
| `ORDERINGS/` | `cto_` | Term orderings (KBO, LPO, etc.) |
| `CLAUSES/` | `ccl_` | Clauses, clause sets, rewriting, indexing |
| `PROPOSITIONAL/` | — | Propositional/SAT reasoning |
| `LEARN/` | `cle_` | Feature extraction, ML data encoding |
| `PCL2/` | — | Proof checking language output/verification |
| `HEURISTICS/` | `che_` | Clause weight functions, auto-scheduling, ENIGMA |
| `CONTROL/` | `cco_` | Main saturation loop, inference rules |
| `PROVER/` | — | `main()` entry points, option parsing |

### Key files for HO work

- `TERMS/cte_lambda.c` — lambda term manipulation, eta/beta reduction
- `TERMS/cte_ho_bindings.c` — HO variable bindings
- `TERMS/cte_ho_csu.c` — higher-order constrained unification
- `CONTROL/cco_ho_inferences.c` — HO-specific inferences (`ImmediateClausification`, `ResolveFlexClause`, `NormalizeEquations`, `BooleanSimplification`, etc.)
- `CLAUSES/ccl_rewrite.c` — rewriting, including `MakeRewrittenTerm` (applies lambda normalization in HO mode)

### First-order vs. higher-order differences

HO extends FO with:
- Lambda abstractions and partial application
- De Bruijn indices for lambda-bound variables
- Applied free variables: terms of the form `X a₁ … aₙ` where `X` is a free variable — these **only exist in HO** and are the source of most HO-specific bugs
- `MakeRewrittenTerm` calls lambda normalization (beta/eta reduction) after substitution in HO mode but not FO mode — this creates subtle differences in what "the rewritten term" is

When a bug is HO-only, the first question is whether applied free variables or lambda normalization is involved.

### Term banks

Terms are hash-consed into a term bank (`TB_p`). The `owner_bank` field on each term points back to its bank. Functions that copy terms (e.g., `TermTopCopyWithoutArgs`) zero out `owner_bank` — callers that need the bank on the copy must restore it via `TermSetBank(copy, TermGetBank(source))`. See `bugs/ALG247^2.md` for an example of this pattern.

There are two live banks at runtime: `state->terms` (the main bank) and `state->tmp_terms` (scratch space). They have different GC regimes: `TBGCCollect(state->terms)` is a full mark-and-sweep rooted at all clause sets; `TBGCSweep(state->tmp_terms)` is called every saturation cycle and keeps only `true_term/false_term/min_terms` — everything else in `tmp_terms` is freed. A term from `state->terms` must never hold a pointer into `tmp_terms` across a sweep boundary.

The WHNF cache (`binding_cache` field on lambda-headed phony apps) is one such pointer. `WHNF_step` uses `TermGetBank(t)` to decide which bank to allocate the cached result in, so `owner_bank` must be correct at call time. `TBInsertNoProps` temporarily sets `owner_bank` to the destination bank (the "cheat") before calling `WHNF_deref` — if the destination is `tmp_terms`, the cache gets a cross-bank pointer. The guard added in bug004 clears `binding_cache` after restoring `owner_bank` when banks differ. See `bugs/bug004-whnf-cache-cross-bank-pollution.md`.

### Heuristics and weight functions

- Each weight function (e.g., `ConjectureTermPrefixWeight`) is registered in `HEURISTICS/` and lazily initializes state on first call
- `che_new_autoschedule.c` — auto-scheduling: selects a sequence of strategies based on problem features
- Option handling: `OptionCodes` enum in `PROVER/e_options.h`, processed in `PROVER/eprover.c:process_options`
- Printing for debugging: `TBPrintTerm`/`TBPrintTermFull` (`TERMS/cte_termbanks.c`), `TermPrintFO`/`TermPrintHO` (`TERMS/cte_termfunc.c`), `ClausePrint` (`CLAUSES/ccl_clauses.c`)

### Similarity-based weight functions (yan's)

All six functions measure similarity of a clause to the conjecture and are described in
"Extending E Prover with Similarity Based Clause Selection Strategies" (Jakubův & Urban,
CICM 2016, `bugs/e-heuristics.pdf`).

**Common arguments** shared by all six:
- `v` — variable normalization: `0`=α-normalized (consistent left-to-right naming), `1`=★ (all vars unified to one)
- `r` — RelatedTerms set: `1`=conjecture terms (`ter`), `2`=conjecture subterms (`sub`), `3`=top-level generalizations (`top`), `4`=all generalizations (`gen`)
- `e` — base-weight extension: `0`=value directly (`1`), `1`=sum over subterms (`Σ`), `2`=max over subterms (`∨`)

**The six functions:**

| Name | File | Description | Extra args |
|------|------|-------------|------------|
| `ConjectureTermWeight` | `che_termweight.c` | Counts subterms shared with conjecture; like conjecture symbol weight but on terms | γ_conj, δ_f, δ_c, δ_p, δ_v |
| `ConjectureFrequencyWeight` (Tfldf) | `che_tfidfweight.c` | TF-IDF: downweights terms common across many clauses | δ_doc (`ax`/`pro`) |
| `ConjectureTermPrefixWeight` (Pref) | `che_prefixweight.c` | Prefix match on term-as-symbol-sequence; partial matches score by length of longest shared prefix | δ_match, δ_miss |
| `ConjectureLevWeight` (Lev) | `che_levweight.c` | Levenshtein edit distance on term-as-symbol-sequence | δ_ins, δ_del, δ_ch |
| `ConjectureTreeWeight` (Ted) | `che_treeweight.c` | Tree edit distance (insert/delete node, rename label) | δ_ins, δ_del, δ_ch |
| `ConjectureStrucWeight` (Struc) | `che_strucweight.c` | Structural distance via generalization/instantiation operations | δ_miss, γ_inst, γ_gen |

Best configurations from benchmarks (2078 MPTP bushy problems, 5 s limit): Lev and Struc
solve the most (~830–840), with Struc showing highest `%ref+` gain (~17%) over baseline.
Pref is fastest to evaluate. Lev/Ted are O(n²)/O(n³) in term size; Struc is O(n³).

## Navigating Key Entry Points

- Main saturation loop: `cco_proofproc.c` — `ProofStateInit`, `ProofStateProcess`
- Inference rules: `CONTROL/cco_inferences.c` (FO), `CONTROL/cco_ho_inferences.c` (HO)
- Rewriting: `CLAUSES/ccl_rewrite.c` — `ClauseRewrite`, `TermRewrite`, `MakeRewrittenTerm`
- Term weight: `TERMS/cte_termfunc.c` — `TermWeightCompute`, `TermStandardWeight`
