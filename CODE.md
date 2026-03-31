# CODE.md

Notes on E Prover internals, accumulated during bug-fixing work.

---

## Representation of HO Terms

HO terms are `Term_p` (`struct termcell`, `TERMS/cte_termtypes.h`). The key fields are
`f_code` (top symbol) and the `TPIsDBVar` property flag. There are six distinct kinds:

| Kind | `f_code` | `TPIsDBVar` | `arity` | Notes |
|---|---|---|---|---|
| Free variable | `< 0` | — | 0 | even=X, odd=Y (alt vars) |
| DB variable `db(n)` | `> 0` (= index) | **set** | 0 | shared in `db_vars` bank |
| Phony app `$@_var` | 17 | — | ≥ 2 | `args[0]` is head |
| Named lambda | 18 | — | 1 | surface lambda (from parsing) |
| DB lambda `$db_lam` | 19 | — | 1 | internal, used post-clausification |
| Normal symbol | `> 0`, ≠17–19 | — | ≥ 0 | signature lookup |

### Free variables

Same as FO: `f_code < 0`. Normal vars have **even** negative codes (`-2, -4, -6, ...`,
printed `X1, X2, ...`), "alt" vars have **odd** negative codes (`-1, -3, -5, ...`, printed
`Y1, Y2, ...`). Detected by `TermIsFreeVar(t)` ↔ `t->f_code < 0`.

### De Bruijn variables — `db(n)`

Not distinguished by `f_code` sign. Instead, DB vars have a **positive** `f_code`
(encoding the DB index/level) **and** the property flag `TPIsDBVar` set. They are shared
in a `DBVarBank` (`db_vars` field on the term bank), a curried `index → type → term` map.

```c
#define TermIsDBVar(term)  (QueryProp((term), TPIsDBVar))
```

Always `arity == 0`. Use `TBRequestDBVar(bank, type, idx)` to obtain one.

### Application node — phony app (`SIG_PHONY_APP_CODE = 17`)

In HO mode, curried application `f a₁ … aₙ` where `f` cannot be flattened into a
single signature symbol is represented as a **phony app** node:

```
f_code == SIG_PHONY_APP_CODE (17)
args[0] = head (the function being applied)
args[1..n] = arguments
```

Three sub-cases, all `f_code == 17`, distinguished by `args[0]`:

| Macro | Condition | Example |
|---|---|---|
| `TermIsAppliedFreeVar` | `args[0]->f_code < 0` | `X @ a₁ @ a₂` |
| `TermIsAppliedDBVar`   | `TermIsDBVar(args[0])` | `db(0) @ a` |
| lambda-headed phony app | `args[0]` is a lambda | `(λx.body) @ a` |

Lambda-headed phony apps are the ones that carry the **WHNF cache** (`binding_cache`
field). See `TERMS/cte_lambda.c:whnf_step_uncached` and
`bugs/bug004-whnf-cache-cross-bank-pollution.md`.

### Key predicates

```c
TermIsFreeVar(t)          // f_code < 0
TermIsDBVar(t)            // TPIsDBVar flag set
TermIsPhonyApp(t)         // f_code == 17, not a DB var
TermIsAppliedFreeVar(t)   // phony app with free-var head
TermIsAppliedDBVar(t)     // phony app with DB-var head
TermIsLambda(t)           // f_code == 18 or 19
TermIsDBLambda(t)         // f_code == 19
```

The confusing part: DB vars "look like" regular symbols (`f_code > 0`) but are flagged
with `TPIsDBVar`. That is why all the predicates check `!TermIsDBVar(term)` before
testing `f_code == SIG_PHONY_APP_CODE`.

---

## Term Banks

A **term bank** (`TB_p`, `struct tbcell`, `TERMS/cte_termbanks.h`) is a hash-cons pool:
every shared term has exactly one canonical cell, identified by its `entry_no`. The
structural hash is keyed on `f_code`, masked properties, and the `entry_no`s of arguments
(splay tree in `term_store`).

### Key fields on `TBCell`

```c
Sig_p         sig;        // signature (shared)
VarBank_p     vars;       // free variables (shared per bank)
DBVarBank_p   db_vars;    // de Bruijn variables (shared per bank)
Term_p        true_term;  // $true constant
Term_p        false_term; // $false constant
PDArray_p     min_terms;  // one minimal term per sort (used as GC roots)
GCAdmin_p     gc;         // registered clause/formula sets (GC roots for TBGCCollect)
TermCellStoreCell term_store; // the hash-cons store
```

### Inserting terms

- `TBTermTopInsert(bank, t)` — inserts an **unshared** top cell (args already shared);
  returns the canonical shared cell (which may not be `t` if an equal term already exists).
- `TBInsert(bank, term, deref)` — recursive: copies an entire term tree into `bank`,
  dereferencing variable bindings along the way. Used for moving terms between banks.
- `TBInsertNoProps(bank, term, deref)` — same but strips all term properties (used when
  copying into `tmp_terms`, where properties from the source bank are meaningless).

### The two live banks

At runtime there are exactly two banks:

| Bank | Field | GC trigger | GC roots |
|---|---|---|---|
| Main | `state->terms` | periodically in saturation loop | all registered clause/formula sets via `gc` |
| Scratch | `state->tmp_terms` | every saturation cycle | `true_term`, `false_term`, `min_terms` only |

`TBGCCollect(state->terms)` is a full mark-and-sweep: marks everything reachable from
every registered clause set and formula set, then sweeps the store.

`TBGCSweep(state->tmp_terms)` is called without any prior marking of clause sets — it
only auto-marks `true_term`, `false_term`, and `min_terms`. Everything else in `tmp_terms`
is freed. This is intentional: `tmp_terms` is pure scratch space and is expected to be
fully cleared each cycle.

**Critical invariant:** a term in `state->terms` must never hold a pointer into
`state->tmp_terms` across a `TBGCSweep(tmp_terms)` call. The WHNF cache (`binding_cache`)
is one such pointer — see `bugs/bug004-whnf-cache-cross-bank-pollution.md` for what
happens when this invariant is violated.

### Variables are per-bank

`vars` (free variables) and `db_vars` (de Bruijn variables) belong to the bank. When
copying a term into a different bank, `TBInsert`/`TBInsertNoProps` re-allocate variables
in the destination bank's `vars`/`db_vars`. The `owner_bank` field on each term cell
points back to the bank it was inserted into. `TermTopCopyWithoutArgs` (used internally)
zeroes `owner_bank` — callers that need it on the copy must restore it via
`TermSetBank(copy, TermGetBank(source))`.

---

## Beta-Reduction

All beta-reduction lives in `TERMS/cte_lambda.c`. Terms are in **De Bruijn form** by the
time beta-reduction runs (named lambdas are converted to DB form by `LambdaNamedToDB`
during clausification).

### Lambda representation

A lambda `λ(body)` is a `SIG_DB_LAMBDA_CODE` (19) node with two args:

```
args[0] = DB variable placeholder (type only, always a DB var)
args[1] = body
```

An application `(λ body) @ a₁ @ a₂` is a lambda-headed phony app:

```
f_code = 17, args[0] = lambda node, args[1] = a₁, args[2] = a₂
```

### WHNF step — `WHNF_step` / `whnf_step_uncached`

One step of **weak head normal form** reduction: given `(λ⁺ body) @ a₁ … aₙ`, consume
as many leading lambdas as there are arguments (or until body has no more lambdas), bind
each DB var to the corresponding argument, substitute into the body via
`replace_bound_vars`, and re-apply any leftover arguments.

The result is cached in `t->binding_cache` (on the phony app node). Subsequent calls to
`WHNF_step` with the same node return the cached result without recomputing.

Internally, substitution uses **DB index arithmetic**:

- `replace_bound_vars` walks the body; for a DB var with index `i` in scope of
  `total_bound` lambdas: if `i < total_bound` it replaces with the bound argument
  (shifted by current depth via `ShiftDB`); if `i ≥ total_bound` it adjusts the index
  down by `total_bound` (the lambda binders are consumed).
- `ShiftDB` (`do_shift_db`) adjusts free DB indices when a term is moved under a
  different number of binders.

### Full beta normalization — `BetaNormalizeDB` / `do_beta_normalize_db`

`do_beta_normalize_db` is a recursive traversal that normalizes all redexes, not just the
head. It uses `TPIsBetaReducible` as an early-exit flag (set on any term with a
lambda-headed subterm) to avoid descending into already-normal subtrees.

```
do_beta_normalize_db(bank, t):
  if t is lambda-headed phony app:
    res = WHNF_step(bank, t)        // one step at head
    if res is still beta-reducible:
      res = do_beta_normalize_db(bank, res)   // recurse
  elif t is a lambda:
    reduce body recursively, re-wrap with CloseWithDBVar
  elif t has no beta-reducible subterms:
    return t unchanged
  else:
    copy top, recurse into each arg, re-insert if changed
```

All intermediate results are immediately inserted into the bank (`TBTermTopInsert`) so
the invariant "all args of a shared term are shared" is maintained throughout.

### WHNF deref — `WHNF_deref`

`WHNF_deref` combines variable dereferencing (following `binding` chains) with WHNF
reduction until the head is in weak head normal form. Used in `TBInsertNoProps` to
normalize terms being copied into a bank.

### Eta reduction — `LambdaEtaReduceDB`

Reduces `λx. (f @ x)` → `f` when `x` does not appear free in `f`. Implemented by
`drop_args` (strips trailing args from a term matching the DB-var pattern). The global
`eta_norm` function pointer is set to `LambdaEtaReduceDB` and called as part of
`LambdaNormalizeDB` (the combined beta+eta normalizer used in rewriting).

### Entry points used by the rewriter

```c
BetaNormalizeDB(bank, t)    // full beta normalization
LambdaEtaReduceDB(bank, t)  // eta reduction only
LambdaNormalizeDB(bank, t)  // beta then eta (called by MakeRewrittenTerm in HO mode)
WHNF_deref(t)               // head normalization + deref (used in TBInsertNoProps)
```
