# Working files: non-Boolean formula at formula position

Test artifacts for the fix of
`bugs/bug005-nonbool-toplevel-lambda-segfault.md`: a THF annotated formula
whose type is not `$o` (e.g. `^ [X: $o] : ( p & X )`, of type `$o > $o`) was
not rejected, reached clausification and crashed `TFormulaNNF`. See
`PULL-REQUEST.md` for the full write-up.

Code changes:

1. `TFormulaHasNonBoolSubForm()` in `CLAUSES/ccl_tformulae.c`, called from
   `WFormulaTSTPParse()` in `CLAUSES/ccl_formula_wrapper.c`, right next to the
   existing free-variable check.
2. Four stray `assert(false)` removed in `TERMS/cte_typecheck.c` — they
   aborted debug builds on ordinary type errors.
3. A NULL guard in `applied_tform_tstp_parse()` (`CLAUSES/ccl_tformulae.c`)
   for a second, independent segfault found while testing: `p @ q`, i.e.
   applying an argument to a Boolean atom.

## Contents

| path | what |
|---|---|
| `PULL-REQUEST.md` | PR description for upstream |
| `cases/*.p` | 39 targeted cases, one formula per file |
| `run.sh` | prints `in:`/`rc:`/`out:` triples for the cases |
| `baseline.txt` | case output *before* the change (release build) |
| `after.txt` | case output *after* the change (release build) |
| `corpus-*.txt` | full `--print-formulas --syntax-only` dumps of all 115 `.p`/`.tptp`/`.lop` files in the tree, patched vs unpatched |

`corpus-unpatched.txt` and `corpus-patched.txt` differ in exactly three
files, all of them deliberately ill-typed bug reproductions:
`bugs/bug005-nonbool-toplevel-lambda-segfault.p` and the two lambda cases
`32`/`90` from `../parse/cases/`. The other 112 are byte-identical, exit
codes included. (The dumps predate `cases/` below, so they do not include
it — that keeps the comparison apples-to-apples.)

## Usage

```sh
bash bugs/dust/nonbool/run.sh                       # all cases
bash bugs/dust/nonbool/run.sh 01-lam-and-var2.p     # one case
diff bugs/dust/nonbool/baseline.txt bugs/dust/nonbool/after.txt

# leak check (needs a MEMDEBUG build, see Makefile.vars) - accepted cases only,
# the rejected ones exit via Error() and so print no footer
bash bugs/dust/parse/memcheck.sh --auto -- bugs/dust/nonbool/cases/2*.p
```

Cases `30`/`32` are only interesting in a build with assertions enabled
(`NODEBUG` commented out in `Makefile.vars`); in a release build the
`assert(false)` they used to hit is compiled out anyway.

## Case numbering

* `01`–`19` **ill-typed, must be rejected.** `01` is the shape from
  `bug005`; `03`, `07`, `12`, `14` were the other SIGSEGVs; `02`, `04`,
  `05`, `08`, `09`, `10`, `15` were silently *accepted* and answered
  "Unsatisfiable". `08`–`13` have top-level type `$o` and are only ill-typed
  below a connective, so a top-level-only check would miss them. `15` is the
  shape that the binder/equation parse change newly made reachable.
  `16`–`19` hide the ill-typed connective *inside a term* — below a lambda
  that is itself an application argument (`16`–`18`) or inside an `$ite`
  branch (`19`). These are why the walk cannot stop at the formula skeleton;
  `17` was a SIGSEGV, `18`/`19` were silently accepted.
* `20`–`29` **well-typed, must keep working.** Logical symbols applied via
  `@` (`20`, `21`), Boolean and predicate variables at atom position (`22`,
  `23`, `28`), `$ite` (`24`), equations under a quantifier (`25`, `26`), an
  application that *is* Boolean (`27`), and the beta redex that makes case
  `01` well-typed again (`29`).
* `40`–`41` **well-typed formulas nested inside terms** — the counterparts of
  `16`–`19`, which the deeper walk must still accept.
* `30`–`37` **type errors, must terminate gracefully.**
  `30` (argument mismatch), `33` (over-applied connective) and `34` (argument
  mismatch under a variable head) each reach one of the removed
  `assert(false)` sites in `TypeInferSort()` and used to abort with SIGABRT in
  a debug build. `31` is the FO analogue and `32` the parser's own arg-count
  guard; both were already graceful. `35`–`37` are the second segfault —
  applying something to a Boolean atom, an equation, or `$true`.
