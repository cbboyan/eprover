# Bug: Non-Boolean top-level lambda segfaults in clausification

**File:** `bugs/bug005-nonbool-toplevel-lambda-segfault.p` (minimal — 2 formulae)

**Trigger:** `bash bugs/bug005-nonbool-toplevel-lambda-segfault.sh`

**Status:** OPEN — root cause not yet established. Documented only.

---

## Symptom

Deterministic — fails on every run.

**Release build** (`-O3 -DNDEBUG`):

```
$ eprover-ho --auto bug005-nonbool-toplevel-lambda-segfault.p
Segmentation fault (core dumped)      # exit 139
```

```
Program received signal SIGSEGV, Segmentation fault.
#0  TFormulaNNF ()                  CLAUSES/ccl_tcnf.c
#1  TFormulaNNF ()                  CLAUSES/ccl_tcnf.c
#2  WTFormulaConjunctiveNF3 ()
#3  WFormulaCNF2 ()
#4  FormulaSetCNF2 ()
#5  main ()
```

**Debug build** (`MEMDEBUG`/`DEBUGGER` on, `NODEBUG` off in `Makefile.vars`) —
sharper, and it localises the fault precisely:

```
eprover-ho: ccl_tcnf.c:2040: TFormulaNNF: Assertion `form && terms' failed.
Aborted (core dumped)                 # exit 134
```

So `form` is **NULL** on the recursive entry to `TFormulaNNF`. The segfault in
the release build is that same NULL being dereferenced a few lines later. The
question to answer is which caller passes NULL: the two-deep frame in the
release backtrace points at the `and_code`/`or_code` recursion (see below).

## Minimal input

```tptp
thf(p_decl,type,p: $o).
thf(c,axiom, ^ [X: $o] : ( p & X ) ).
```

## Why this is a bug

The axiom is **ill-typed**. A `thf` annotated formula must have type `$o`, but
`^ [X: $o] : ( p & X )` has type `$o > $o`. E should reject it with a type
error (it does report exactly that kind of error in argument positions, e.g.
`Type mismatch in argument #1 of hq @ ...: expected $o > $o but got $o`).
Instead the un-applied lambda survives parsing and reaches clausification.

Whatever the eventual fix, **the code must not segfault** on ill-typed input.

## Trigger conditions

The bound variable must occur as the **second** argument of a logical operator
inside a top-level lambda. Argument order matters:

| formula | result |
|---|---|
| `^ [X: $o] : ( p & X )` | **SIGSEGV** |
| `^ [X: $o] : ( p = X )` | **SIGSEGV** |
| `^ [X: $o] : ( X & p )` | ok (exit 0) |
| `^ [X: $o] : ( X = p )` | ok (exit 0) |
| `^ [X: $o] : ( p = q )` | ok (exit 0) — no bound variable |
| `^ [X: $o] : p`         | ok (exit 0) — no logical operator |
| `! [X: $o] : ( p = X )` | ok (exit 0) — quantifier, not lambda |

## Preliminary analysis (unverified)

`TFormulaNNF` (`CLAUSES/ccl_tcnf.c:2035`) has no case for a lambda
(`SIG_NAMED_LAMBDA_CODE`) at formula position. The `and_code`/`or_code` branch
(`ccl_tcnf.c:2070-2083`) recurses unconditionally into both arguments:

```c
handle  = TFormulaNNF(terms, form->args[0], polarity);
handle2 = TFormulaNNF(terms, form->args[1], polarity);
```

so it descends into the De Bruijn-bound variable, which is not a formula. The
two-deep `TFormulaNNF` frames in the backtrace are consistent with this, as is
the `form && terms` assertion failure: one of `form->args[0]`/`args[1]` is
NULL for the lambda-bodied term. The
final `else` branch (`ccl_tcnf.c:2096`) calls `EncodePredicateAsEqn` on
whatever it gets, and directly above it sits an assertion that was
**commented out**:

```c
/* assert(TFormulaIsLiteral(terms->sig, form)
        && "Top level term not in normal form"); */
```

i.e. this failure mode was anticipated. Re-enabling that assertion in a debug
build is probably the fastest way to pin down the exact point of failure.

Two plausible fix directions, to be decided later:

1. **Reject earlier** — type-check that an annotated formula has type `$o`
   right after parsing. This is the real fix; the input is simply invalid.
2. **Harden `TFormulaNNF`** — handle (or explicitly refuse) non-Boolean
   subterms instead of recursing blindly.

## Relation to the binder/equation parser fix

This bug is **pre-existing and independent** of the binder-scoping change in
`CLAUSES/ccl_tformulae.c`. Verified by rebuilding without that patch: the
input above segfaults identically. Its parse is unaffected by the patch — the
body is parenthesized, so the absorption path is never entered.

The parser fix does make the crash reachable from *new* input, though:
`^ [X: $o] : p = X` (unparenthesized) previously failed with "Formula has free
variables" and now parses to the same crashing shape.
