# TODO — HO ENIGMA Feature Encoding

Issues found in `HEURISTICS/che_enigmaticvectors.c` (enigma branch).

---

## Issue 1 — Head of phony app invisible in vertical walks (High)

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

## Issue 2 — Head treated as sibling in horizontal walks (Medium)

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

## Issue 3 — DB variable index not encoded (Design choice)

All DB vars map to `"^"` (or `"^:type"` in typed mode) regardless of their index.
This means `λx.λy. x` and `λx.λy. y` produce identical features for the bound vars —
depth/scope within nested lambdas is lost. Analogous to how all free vars become `*`.

Probably intentional (scope-independence), but worth reconsidering for HO problems
where lambda depth matters.

---

## Issue 4 — DB lambda placeholder visited (Low / typed-mode useful)

`$db_lam(^, body)`: `args[0]` is the placeholder DB var (carries type, no semantic
content). `update_term` visits it, generating path features at `[..., $db_lam, ^]`.

- **Untyped mode**: all placeholders produce identical `^` — uniform noise.
- **Typed mode**: `symbol_string` returns `"^:type"`, encoding the bound variable's
  type — this is actually useful.

No fix needed in typed mode. In untyped mode, consider skipping `args[0]` of a DB
lambda (only recurse into `args[1]` = body).

---

## Non-issue — Lambda-headed phony apps

`(λ body) @ a` (unreduced beta-redex) is encoded as any other phony app. These should
not appear in clauses after `BetaNormalizeDB` runs during clausification. Not a
practical concern.

---

## Summary Table

| # | Issue | Severity | File | Fix |
|---|-------|----------|------|-----|
| 1 | Head invisible in vertical walks | High | `update_term`, `update_verts` | Use head as path node for phony apps |
| 2 | Head as sibling in horizontal walks | Medium | `update_horiz` | Use head as root, `args[1..n]` as children |
| 3 | DB var index not encoded | Design choice | `symbol_string`, `FCODE` | Reconsider if scope depth matters |
| 4 | DB lambda placeholder traversed | Low | `update_term` | Skip `args[0]` of `$db_lam` in untyped mode |
