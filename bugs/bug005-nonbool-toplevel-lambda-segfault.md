# Bug: Non-Boolean formula at formula position segfaults in clausification

**File:** `bugs/bug005-nonbool-toplevel-lambda-segfault.p` (minimal — 2 formulae)

**Trigger:** `bash bugs/bug005-nonbool-toplevel-lambda-segfault.sh`

**Status:** FIXED — rejected at parse time with a normal syntax error (exit 3).

**Fix:** `TFormulaHasNonBoolSubForm()` in `CLAUSES/ccl_tformulae.c`, called
from `WFormulaTSTPParse()` (`CLAUSES/ccl_formula_wrapper.c`) next to the
existing free-variable check. Working files and the upstream write-up are in
`bugs/dust/nonbool/`.

The title says "top-level lambda" because that is how it was found, but the
defect was neither lambda-specific nor top-level-specific — see
"Actual extent" below.

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

## Actual extent (established while fixing)

The table above only covers top-level lambdas. The real rule is: **any**
non-Boolean term reaching formula position misbehaves, and the symptom
depends on the shape. With `p,q: $o`, `a: $i`, `f: $i>$i`, `g: $i>$o`:

| formula | old result |
|---|---|
| `f @ a` | **SIGSEGV** — no lambda involved at all |
| `p \| (f @ a)` | **SIGSEGV** — top-level type is `$o`, only the arg is bad |
| `^ [X: $o] : ( X & p )` | accepted, "SZS status Unsatisfiable" |
| `^ [X: $o] : p` | accepted, "SZS status Unsatisfiable" |
| `p & (^ [X: $o] : X)` | accepted, "SZS status Unsatisfiable" |
| `~ (^ [X: $o] : X)` | accepted, "SZS status Unsatisfiable" |
| `! [X: $o] : (^ [Y: $o] : Y)` | accepted, "SZS status Unsatisfiable" |
| `p & a` | "type error", exit 4, but only during preprocessing |

So the "ok (exit 0)" entries in the trigger table above were *not* ok — they
were silent wrong answers on ill-typed input, which is worse than the crash.
`f @ a` prints as `~(a)` under `--print-formulas`, because the formula
printer reinterprets the phony app as a connective.

Because cases like `p | (f @ a)` are ill-typed only *below* a connective, a
top-level-only type check is not enough; the fix walks the whole formula
skeleton.

## Preliminary analysis (unverified at the time; confirmed)

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

Two plausible fix directions were considered:

1. **Reject earlier** — type-check that an annotated formula has type `$o`
   right after parsing. This is the real fix; the input is simply invalid.
2. **Harden `TFormulaNNF`** — handle (or explicitly refuse) non-Boolean
   subterms instead of recursing blindly.

Direction 1 was taken, but "the annotated formula has type `$o`" is not by
itself sufficient — `p | (f @ a)` has type `$o` and still segfaulted. The
check therefore recurses over the formula skeleton (the arguments of the
propositional connectives, tested with the existing `TFormulaHasSubForm1/2()`
macros, and the body of a quantifier), and stops at anything that is a term
rather than a formula: the sides of a (dis)equation, the arguments of an
application, the body of a lambda, `$ite`/`$let`.

`TFormulaNNF` was left alone. With the parser rejecting the bad input, the
commented-out assertion above could now plausibly be re-enabled, but that was
not attempted.

## Related: type errors abort in debug builds

Found while fixing this. `TypeInferSort()` (`TERMS/cte_typecheck.c`) had
`assert(false)` on the line before each of its four `TI_ERROR("Type error")`
calls. `TI_ERROR` always ends in `exit()`, so the assertion could only turn a
clean, well-diagnosed type error into a SIGABRT in any build with assertions
enabled:

```
eprover-ho: cte_typecheck.c:327: TypeInferSort: Assertion `false' failed.
```

All four were removed, so ill-typed input now exits with code 3 in debug
builds too. Reproductions: `bugs/dust/nonbool/cases/30-argtype-mismatch.p`,
`33-overapplied-connective.p`, `34-var-head-argtype.p` (one per reachable
site; the fourth, "Type mismatch for ... and type ...", could not be reached
from any input — the parser rejects those earlier).

## Related: a second segfault, applying to a Boolean atom

Also found while fixing this, and **independent** of it — it crashes inside
`TFormulaTSTPParse()`, before the new Boolean check runs:

```tptp
thf(c,axiom, p @ q ).      % p, q : $o
```

```
eprover-ho: ccl_tformulae.c:683: applied_tform_tstp_parse: Assertion `hd_type' failed.
Segmentation fault (core dumped)     # release build
```

`GetHeadType()` returns NULL for a polymorphic head, and an equation is
polymorphic — the head here *is* an equation, because `EncodePredicateAsEqn()`
has just rewritten the atom `p` into `$eq(p,$true)`. The NULL then reaches
`TypeGetMaxArity(hd_type)`. Same for `(a = a) @ p` and `$true @ p`.

Fixed by replacing `assert(hd_type)` with the graceful "Too many arguments
applied to the term" error the function already raises two lines further
down. Reproductions: `bugs/dust/nonbool/cases/35-apply-bool-atom.p`,
`36-apply-equation.p`, `37-apply-true.p`.

## Relation to the binder/equation parser fix

This bug is **pre-existing and independent** of the binder-scoping change in
`CLAUSES/ccl_tformulae.c`. Verified by rebuilding without that patch: the
input above segfaults identically. Its parse is unaffected by the patch — the
body is parenthesized, so the absorption path is never entered.

The parser fix does make the crash reachable from *new* input, though:
`^ [X: $o] : p = X` (unparenthesized) previously failed with "Formula has free
variables" and now parses to the same crashing shape.

One consequence worth recording: unparenthesized lambda extensionality,
`^ [X: $i] : (f @ X) = f`, now parses as `^ [X: $i] : ((f @ X) = f)`, type
`$i > $o`, and is therefore rejected by this fix. That matches the TPTP BNF
(the body of a binder is a `<thf_unit_formula>`, and `s = t` is one). The
parenthesized form `( ^ [X: $i] : (f @ X) ) = f` is unaffected.
See `bugs/dust/nonbool/cases/15-lam-bare-eq.p`.
