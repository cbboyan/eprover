# EHO.md

Notes connecting "Extending a High-Performance Prover to Higher-Order Logic"
(Vukmirović, Blanchette, Schulz — TACAS 2023) to the λE source code.

**Paper sections:** §2 Logic · §3 Terms · §4 Unification/Matching/Indexing ·
§5 Preprocessing/Calculus · §6 Evaluation

---

## §3 Terms — Representation

### Phony app and spine form (paper p. 113)

The paper says λE uses a **flattened, spine notation**: `f`, `f a`, and `f a b` are
represented as `f`, `f(a)`, and `f(a,b)` — i.e., the head and its applied arguments
are flattened into a single cell. Applied free variables are an exception: `X a b` is
represented as the cell `@(X, a, b)` using the distinguished symbol `@` of variable
arity.

In the code (`TERMS/cte_termtypes.h`, `TERMS/cte_signature.h`):
- `SIG_PHONY_APP_CODE = 17` is the `@` symbol.
- `f(a,b)` is a single cell with `f_code = f` and `args = [a, b]`.
- `X a b` is a phony-app cell `f_code = 17`, `args = [X, a, b]`.
- Function `flatten_and_make_shared` (`cte_lambda.c`) handles the case where
  beta-reduction or eta-reduction produces a `@(f, ...)` with a non-variable,
  non-lambda head that needs to be flattened back into a normal cell.

### De Bruijn indices (paper p. 113)

The paper explains the design choice: use **positive** `f_code` for DB indices
(same range as function symbols) but distinguish them by the `IsDBVar` property flag.
This avoids a negative-code collision with free variables.

In the code: `TPIsDBVar = 1<<23` in `TermProperties`; `TermIsDBVar(t)` checks this
flag. DB vars are created/shared via `TBRequestDBVar` / `_RequestDBVar`
(`TERMS/cte_dbvars.h`). The `db_vars` field on `TBCell` is a curried `IntMap`
(index → type → term).

### Lambda cells (paper p. 113)

The paper describes each `λx.s` abstraction as a cell with head `LAM` and two
arguments: (1) a DB var of the type of the abstracted variable, and (2) the body `s`.
The first argument is redundant (deducible from the type) but stored for efficiency.

In the code: `SIG_DB_LAMBDA_CODE = 19` is `LAM`. Lambda cells have `arity = 2`:
`args[0]` is the DB var placeholder (type only), `args[1]` is the body.
`CloseWithDBVar(bank, type, body)` constructs a lambda cell.

### Efficiency flags (paper p. 114)

The paper introduces two property bits to avoid traversing FO-only terms during HO
operations:
- `HasBetaReducibleSubterm` — set on any cell that has a lambda-headed `@` as a
  subterm. β-reduction only visits cells with this flag.
- `HasDBSubterm` — set on any cell containing a DB variable. DB-index shifting
  (`ShiftDB`) and substitution only visit cells with this flag.

In the code (`TERMS/cte_termtypes.h`):
- `TPIsBetaReducible = 1<<21` — checked by `TermIsBetaReducible(t)`.
- `TPHasDBSubterm = 1<<26` — checked by `TermHasDBSubterm(t)`.
- `do_beta_normalize_db` (`cte_lambda.c:600`) returns `t` immediately when
  `!TermIsBetaReducible(t)`.
- `do_shift_db` (`cte_lambda.c:204`) returns `t` immediately when
  `!TermHasDBSubterm(t)`.

On FO problems these flags are never set, so HO operations have zero overhead.

### Efficient η-reduction (paper p. 114–115)

The paper describes **parallel η-reduction**: rather than stripping one λ at a time,
λE recognizes all trailing matched λ-binders at once and drops them in one step.
The paper also introduces `HasLambda` to skip cells with no λ-subterms during
η-traversal.

In the code:
- `TPHasLambdaSubterm = 1<<24` — `TermHasLambdaSubterm(t)`.
- `TPIsEtaReducible = 1<<22` and `TPHasEtaExpandableSubterm = 1<<25`.
- The parallel strip is implemented by `drop_args` (`cte_lambda.c:59`): given a
  term and a count of trailing args to drop, it removes them in one allocation.
- `LambdaEtaReduceDB` calls `drop_args` after determining the minimal loose DB
  index to decide how many binders can be eliminated simultaneously.

### Boolean terms and quantifiers (paper p. 115)

The paper notes that λE represents quantified formulas as `∀ @ (λx. body)` rather
than `∀(X, body)` during proof search, to avoid issues with Booleans appearing as
subterms in HO contexts.

In the code: `PostCNFEncodeFormulas` (`cte_lambda.c:1291`) converts formulas from
CNF's variable-binding representation to the lambda-encoded form used during
saturation. Internally `do_post_cnf_encode` replaces `∀(X, body)` structures with
`∀ @ (λ db(n). body)`. This is applied via `EqnListMapTerms` after clausification
(`ccl_formulafunc.c:1988`).

---

## §4 Unification, Matching, and Indexing

### CSU iterator (paper p. 116–119)

The paper describes the unification procedure as an **iterator** enumerating a
complete set of unifiers (CSU). The iterator holds five fields: `constraints`,
`bt_state`, `branch_iter`, `steps`, `subst`.

In the code (`TERMS/cte_ho_csu.h`, `cte_ho_csu.c`):

```c
struct csu_iter {
   PQueue_p constraints;    // pairs to unify (= paper's "constraints")
   PStack_p backtrack_info; // saved states for backtracking (= "bt_state")
   StateTag_t current_state;
   Limits_t current_limits; // limits on binding rule applications (= "steps")
   Subst_p subst;           // current substitution (= "subst")
   TB_p bank;
   int steps;               // branch_iter progress counter
   ...
};
```

The paper's `ForwardIter` and `BacktrackIter` pseudofunctions correspond to the
main loop inside `NextCSUElement` (`cte_ho_csu.c:522`). Callers iterate with:

```c
CSUIterator_p unif_iter = CSUIterInit(lhs, rhs, subst, bank);
while(NextCSUElement(unif_iter)) { /* use subst */ }
CSUIterDestroy(unif_iter);
```

This pattern appears in `ccl_eqnresolution.c`, `ccl_factor.c`, and
`cco_paramodulation.c`.

### NormalizeHead (paper p. 117)

The paper's `NormalizeHead` function reduces the top-level β-redex and follows
variable bindings until the head is stable. This corresponds to `WHNF_deref`
(`cte_lambda.c:1205`): it calls `TermDerefAlways` (follow `binding`), then calls
`WHNF_step` while the result is a lambda-headed phony app, recursively.

### Oracles (paper p. 118)

The paper describes two oracles invoked on flex-rigid pairs before generating
bindings: **fixpoint** and **pattern**. They return `UNIFIABLE` (MGU found),
`NOT_UNIFIABLE`, or `NOT_IN_FRAGMENT`.

In the code (`cte_ho_csu.c:350–358`):

```c
OracleUnifResult oracle_res = NOT_IN_FRAGMENT;
if(params->fixpoint_oracle)
   oracle_res = SubstComputeFixpointMgu(lhs, rhs, iter->subst);
if(oracle_res == NOT_IN_FRAGMENT && params->pattern_oracle)
   oracle_res = SubstComputeMguPattern(lhs, rhs, iter->subst);
```

`SubstComputeFixpointMgu` is in `TERMS/cte_fixpoint_unif.c`;
`SubstComputeMguPattern` is in `TERMS/cte_pattern_match_mgu.c`. The oracle flags
`fixpoint_oracle` and `pattern_oracle` are heuristic parameters in `HeuristicParms_p`
(`che_hcb.h`).

### Variable bindings: imitation, projection, identification (paper p. 118)

When oracles fail, `NextBinding` generates the next approximate binding. The paper
lists the binding types: imitation, Huet-style projection, identification, and
elimination (one argument at a time).

In the code: `ComputeNextBinding` (`cte_ho_csu.c:370`) dispatches to
`TERMS/cte_ho_bindings.c`. Limits on non-simple projections, rigid imitations,
identifications, and eliminations are encoded in `Limits_t` and checked against the
`steps` field to enforce termination.

### Pattern matching (paper p. 119)

The paper introduces `HasNonPatternVar` to efficiently determine whether a term is
in the λE pattern class (each free variable either has no args or is applied only to
distinct DB indices). When this flag is absent the fast pattern-matching algorithm
can be used; when present, general matching is tried.

In the code: `TPHasNonPatternVar = 1<<27`. The matching entry point
`NormalizePatternAppVar` (`cte_termbanks.c:2535`) uses this; the guard
`MAYBE_NORMALIZE_APP_VAR` (`cte_termbanks.h:180`) calls it only on top-level
free-variable terms.

### Perfect discrimination trees (paper p. 119–121, Fig. 1)

E uses perfect discrimination trees for demodulator retrieval (finding
generalizations of a query term). The paper describes how λE extends this structure:
- Terms are **serialized** (flattened to a symbol sequence) before storage.
- Applied variable nodes `X s̄ₙ` get dedicated slots in the serialization.
- Lambda nodes `LAM_τ` are serialized with their type.
- Terms are **η-expanded** before serialization so stored terms always have λ-prefixes
  of matching length, enabling the pattern matching algorithm (which equalizes λ-prefix
  lengths) to work correctly.

In the code: `PDTree` (`CLAUSES/ccl_pdtrees.c`). The HO serialization changes and
η-expansion are in the PDTree traversal code. `PDTreeFindNextDemodulator` is called
from `rewrite_with_clause_set` (`ccl_rewrite.c:583`).

Fingerprint indexing (for finding unifiable terms) uses `FPIndex`
(`TERMS/cte_fp_index.h`); the HO variant is called via `FPIndexFindMatchable`
(`ccl_rewrite.c:1196`).

---

## §5 Preprocessing, Calculus, and Extensions

### Preprocessing pipeline (paper p. 121)

The paper describes λE's preprocessing as:
1. Optionally **λ-lift** all λ-abstractions into named definitions (to maintain
   Ehoh compatibility).
2. **FOOL-like unrolling**: remove Boolean subterms in higher-order contexts using
   a clausal normal form transformation.
3. Process **definitions** as rewrite rules via `ClauseSetUnfoldEqDefNormalize`.

In the code (`CLAUSES/ccl_formulafunc.c`):
- FOOL unrolling: `TFormulaUnrollFOOL` → `do_fool_unroll` (line 869). Translates
  `f(p ∧ q)` into an if-then-else encoding. Controlled by the `fool_unroll` flag in
  `FormulaAndClauseSetParse`.
- λ-lifting: `ClauseSetLiftLambdas` / `TFormulaSetLiftLambdas` (lines 2466, 2840)
  via `LiftLambdas` (`ccl_tformulae.c:1086`). Introduces fresh function symbols for
  λ-abstractions.
- Definition unfolding: `ClauseSetUnfoldEqDefNormalize` (`ccl_unfold_defs.c`).

### Argument congruence — AC rule (paper p. 122)

The paper introduces the **argument congruence** rule (AC) to compensate for
disabling the prefix optimization:

```
s ≈ t ∨ C
───────────────  (AC)
s X ≈ t X ∨ C
```

In the code: `ComputeArgCong` (`cco_ho_inferences.c:1721`). Called from the main
inference dispatch in `cco_ho_inferences.c:2689`. Records the derivation step as
`DCArgCong`.

### Negative and positive extensionality — NE and PE (paper p. 122)

```
s ≉ t ∨ C                        s X ≈ t X ∨ C
──────────────────────── NE       ───────────── PE
s (sk X̄) ≉ t (sk X̄) ∨ C         s ≈ t ∨ C
```

In the code:
- `ComputeNegExt` (`cco_ho_inferences.c:1637`), derivation code `DCNegExt`.
- `ComputePosExt` (`cco_ho_inferences.c:1780`), derivation code `DCPosExt`.
- Both are called from the main HO inference loop (`cco_ho_inferences.c:2693–2697`).

### Primitive enumeration (paper p. 123)

The paper describes **primitive instantiation**: instantiating free predicate variables
with approximations (⊥, ⊤, `¬P`, `P ∧ Q`, `P ∨ Q`, or a ground instance).

In the code: `PrimitiveEnumeration` (`cco_ho_inferences.c:2232`) dispatches to
`prim_enum_var` (line 747). Derivation code `DCPrimEnum`. The `PrimEnumMode` enum
controls which approximations are tried.

### ImmediateClausification (paper p. 122–123)

When a formula appears at the top level of a clause during saturation (e.g., after
instantiating a variable with `λx. λy. x ∧ y`), λE re-clausifies it immediately.
The paper calls this "reasoning about formulas."

In the code: `ImmediateClausification` (`cco_ho_inferences.c:2074`). Calls the
full CNF pipeline including `PostCNFEncodeFormulas` (line 264).

### Saturation with multiple unifiers (paper p. 122)

In FO mode, each inference produces one result (one MGU). In HO mode the CSU may
contain multiple unifiers; λE **consumes all elements** of the iterator, generating
one clause per unifier.

In the code: wherever superposition / resolution is performed in HO mode, the
standard `SubstMguComplete` call is replaced (or augmented) with the
`CSUIterInit` / `while(NextCSUElement(...))` pattern (e.g.,
`cco_paramodulation.c:305–306`, `ccl_eqnresolution.c:103–104`).

### Boolean simplification and NormalizeEquations (paper p. 122–123)

`BooleanSimplification` simplifies clauses containing Boolean tautologies like
`p ∧ ⊤ ↔ p`. `NormalizeEquations` rewrites literals of the form
`formula ≈ ⊤` / `formula ≈ ⊥` back into the formula.

In the code (`cco_ho_inferences.c`): `BooleanSimplification` (line 29),
`NormalizeEquations` (line 26). Both are immediate simplifications applied eagerly.

---

## Performance and Gracefulness (paper §6, Fig. 2–3)

The paper reports that λE has **zero overhead** on FO problems — the HO code paths
are all guarded by property flags (`TPIsBetaReducible`, `TPHasDBSubterm`, etc.) that
are never set on FO terms. Fig. 3 shows Ehoh FO (535) ≈ λE FO (537) solved problems,
confirming this. This is the "gracefulness principle" stated in §1.

The main HO gains: +20% TPTP benchmarks, +7% SH benchmarks over Ehoh. On SH
benchmarks (Sledgehammer, largely from Isabelle), λE outperforms all other HO provers
except Zipperposition-coop (which uses Ehoh as a backend).

---

## File Map: Paper Concept → Code Location

| Paper concept | Code location |
|---|---|
| DB variables, `IsDBVar` property | `TERMS/cte_dbvars.h`, `cte_termtypes.h` |
| Phony app `@`, lambda `LAM` | `TERMS/cte_signature.h` (codes 17–19) |
| `HasBetaReducibleSubterm` / `HasDBSubterm` | `TERMS/cte_termtypes.h` (`TPIsBetaReducible`, `TPHasDBSubterm`) |
| β-normalization | `TERMS/cte_lambda.c:do_beta_normalize_db`, `BetaNormalizeDB` |
| η-normalization (parallel) | `TERMS/cte_lambda.c:drop_args`, `LambdaEtaReduceDB` |
| Quantifiers as λ (post-CNF) | `TERMS/cte_lambda.c:PostCNFEncodeFormulas`, `do_post_cnf_encode` |
| Named→DB conversion | `TERMS/cte_lambda.c:do_named_to_db`, `LambdaNamedToDB` |
| FOOL unrolling | `CLAUSES/ccl_formulafunc.c:TFormulaUnrollFOOL`, `do_fool_unroll` |
| λ-lifting | `CLAUSES/ccl_formulafunc.c:ClauseSetLiftLambdas`, `LiftLambdas` |
| CSU iterator | `TERMS/cte_ho_csu.c:CSUIterInit`, `NextCSUElement` |
| Oracles (fixpoint, pattern) | `TERMS/cte_fixpoint_unif.c`, `cte_pattern_match_mgu.c` |
| Bindings (imitation/proj/id) | `TERMS/cte_ho_bindings.c`, `ComputeNextBinding` |
| `HasNonPatternVar` | `TERMS/cte_termtypes.h` (`TPHasNonPatternVar`) |
| Perfect discrimination trees | `CLAUSES/ccl_pdtrees.c`, `PDTreeFindNextDemodulator` |
| Fingerprint indexing | `TERMS/cte_fp_index.h`, `FPIndexFindMatchable` |
| Argument congruence (AC) | `CONTROL/cco_ho_inferences.c:ComputeArgCong` |
| Negative extensionality (NE) | `CONTROL/cco_ho_inferences.c:ComputeNegExt` |
| Positive extensionality (PE) | `CONTROL/cco_ho_inferences.c:ComputePosExt` |
| Primitive instantiation | `CONTROL/cco_ho_inferences.c:PrimitiveEnumeration` |
| ImmediateClausification | `CONTROL/cco_ho_inferences.c:ImmediateClausification` |
| Multiple unifiers in saturation | `CONTROL/cco_paramodulation.c`, `CLAUSES/ccl_eqnresolution.c` |
