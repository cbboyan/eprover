# Reject non-Boolean formulas instead of crashing on them

Fixes the crash reported at the end of the binder/equation-scoping PR: a THF
annotated formula whose type is not `$o` is not rejected, reaches
clausification, and segfaults.

```tptp
thf(p_decl,type,p: $o).
thf(c,axiom, ^ [X: $o] : ( p & X ) ).
```

```
eprover-ho: ccl_tcnf.c:2040: TFormulaNNF: Assertion `form && terms' failed.
Segmentation fault (core dumped)      # release build, exit 139
```

The axiom is ill-typed — an annotated formula must have type `$o`, but this
one has type `$o > $o`.

One commit: `CLAUSES/ccl_tformulae.{c,h}` + `CLAUSES/ccl_formula_wrapper.c` —
the check itself.

(There are two related, independent follow-up fixes found while testing this
one — `#bug005-typecheck-abort-on-type-error` and
`#bug005-apply-to-boolean-atom-segfault` — opened as separate PRs since
neither depends on this one.)

---

## The check

### Scope: bigger than the reported crash

The reported case is a top-level lambda, but the defect is not specific to
lambdas, nor to the top level. Any non-Boolean term that reaches formula
position does something wrong, and *what* it does depends on the shape:

| input (`p,q: $o`, `a: $i`, `f: $i>$i`, `g: $i>$o`) | before |
|---|---|
| `^ [X: $o] : ( p & X )` | **SIGSEGV** |
| `^ [X: $o] : ( p = X )` | **SIGSEGV** |
| `f @ a` | **SIGSEGV** |
| `p \| (f @ a)` | **SIGSEGV** |
| `^ [X: $o] : ( X & p )` | accepted, "SZS status Unsatisfiable" |
| `^ [X: $o] : p` | accepted, "SZS status Unsatisfiable" |
| `^ [X: $i] : ( f @ X )` | accepted, "SZS status Unsatisfiable" |
| `p & (^ [X: $o] : X)` | accepted, "SZS status Unsatisfiable" |
| `~ (^ [X: $o] : X)` | accepted, "SZS status Unsatisfiable" |
| `! [X: $o] : (^ [Y: $o] : Y)` | accepted, "SZS status Unsatisfiable" |
| `p & a` | "type error", exit 4 (during preprocessing) |
| `(g @ a) => a` | "type error", exit 4 (during preprocessing) |
| `hq @ (^[X:$o]:(p & (f @ a)))` | **SIGSEGV** — ill-typed `&` nested inside a lambda |
| `hq @ (^[X:$o]:(p & (^[Y:$o]:Y)))` | accepted, "SZS status GaveUp" |

The silent "Unsatisfiable" answers are arguably worse than the crash. Note
also that `f @ a` involves no lambda at all — it just prints as `~(a)`,
because the formula printer reinterprets the phony app as a connective.

All of these now stop at parse time with a normal syntax error:

```
% Subformula has type $o > $o, but a formula must have type $o
eprover: bug005.p:2:(Column 14): Formula is not Boolean (check parentheses and quantifier precedence)
```

exit code 3 (`SYNTAX_ERROR`), same as any other parse error.

### Why nothing caught it

The type was never *wrong* — it was simply never read. `TypeInferSort()`
infers the lambda's type correctly as `$o > $o` and stores it in
`tform->type`; `TFormulaFCodeAlloc()` rightly leaves it alone, since a lambda
does have an arrow type (it stamps `bool_type` only for `op !=
SIG_NAMED_LAMBDA_CODE`). What is missing is anyone comparing that type
against what the position demands.

E's type checking is entirely *argument*-position based — every diagnostic in
`TypeInferSort()` is of the form "Type mismatch in argument #N of …". A
top-level annotated formula is not an argument of anything, so no check ever
runs on it. In FO the gap is invisible, because `parse_atom()` /
`EqnParseInfix()` build a literal directly and a non-predicate cannot appear
there; in LFHO `parse_ho_atom()` parses a term of any type and the gap opens.
So after parsing, the only validation `WFormulaTSTPParse()` did was
`TFormulaHasFreeVars()` — and the bound `X` passes that.

The formula then reaches clausification, which assumes a Boolean skeleton and
descends into the lambda body as if the binder were a connective. For
`^ [X: $o] : p` the result is not a crash but an outright refutation:

```
$ eprover-ho --auto --proof-object c05.p
thf(c,     axiom, ^[X1:$o]:((p)),     file('c05.p', c)).
thf(c_0_1, plain, (UNNAMED_DBXXX(p)), inference(fof_simplification,[status(thm)],[c])).
thf(c_0_2, plain, ($false),           inference(split_conjunct,[status(thm)],[c_0_1]), ['proof']).
% SZS status Unsatisfiable
```

`UNNAMED_DBXXX` is an internal De Bruijn placeholder leaking into the proof
object; `split_conjunct` then collapses the unrecognised literal to `$false`.
That is how a file whose entire content is an atom `p` reports a refutation.
Whether a given shape crashes or "proves" `$false` just depends on which of
the two the clausifier reaches first — hence `( X & p )` being "fine" and
`( p & X )` segfaulting.

### What has to be checked, and where the walk has to go

Two things make this more than a one-line type assertion on the annotated
formula.

**The top-level type is not enough.** `p & (^[X:$o]:X)` and `p | (f @ a)`
both have top-level type `$o` and are ill-typed only below a connective —
and the second is one of the segfaults. So the arguments of every
propositional connective, and the body of every quantifier, have to be
checked too.

**The walk cannot stop at the formula skeleton.** A term may contain a
formula again — the body of a lambda most obviously, but also an `$ite`
branch. So

```tptp
thf(c,axiom, hq @ (^ [X: $o] : ( p & (f @ a) )) ).   % hq: ($o>$o)>$o
```

hides an ill-typed `&` beneath an application *and* a binder, and still
segfaults if the descent stops at the first term position. The check
therefore descends through everything, and applies the Boolean requirement
only at the positions that have it: `form` itself, connective arguments, and
quantifier bodies. The sides of a (dis)equation, the arguments of an
application and the body of a lambda are not required to be Boolean, but are
still traversed.

The "is this a connective" test already exists in the codebase as
`TFormulaHasSubForm1/2()`, which checks the `FPFOFOp` signature property —
set by `SigInsertFOFOp()` on exactly `~ & | => <=> <~> ~& ~|`. The worker
reuses it:

```c
static TFormula_p tform_find_nonbool(Sig_p sig, TFormula_p form)
{
   Type_p     bool_type = sig->type_bank->bool_type;
   TFormula_p res       = NULL;
   int        i;

   if(TermIsAnyVar(form))
   {
      /* A variable is a perfectly good atom, and has no f_code that
         could be looked up in the signature. */
      return NULL;
   }
   if(TFormulaHasSubForm1(sig, form))
   {
      if(form->args[0]->type != bool_type)
      {
         return form->args[0];
      }
      if(TFormulaHasSubForm2(sig, form) && (form->args[1]->type != bool_type))
      {
         return form->args[1];
      }
   }
   else if(TFormulaIsQuantifiedNL(sig, form) && (form->arity == 2))
   {
      if(form->args[1]->type != bool_type)
      {
         return form->args[1];
      }
   }
   for(i=0; i<form->arity; i++)
   {
      res = tform_find_nonbool(sig, form->args[i]);
      if(res)
      {
         break;
      }
   }
   return res;
}

TFormula_p TFormulaHasNonBoolSubForm(Sig_p sig, TFormula_p form)
{
   if(form->type != sig->type_bank->bool_type)
   {
      return form;
   }
   return tform_find_nonbool(sig, form);
}
```

It returns the offending subformula (or NULL), mirroring
`TFormulaHasFreeVars()` immediately above it. The traversal is linear in the
size of the parsed formula — it runs once per input formula, on a structure
that mirrors the input text, so it cannot blow up on sharing.

### Where it is called

One place — `WFormulaTSTPParse()`, immediately after the existing
free-variable check, which is the established "reject a badly-shaped input
formula" spot and already has the source position at hand:

```c
 if(TFormulaHasFreeVars(terms, tform))
 {
    Error("%s Formula has free variables ...", SYNTAX_ERROR, PosRep(...));
 }
+nonbool = TFormulaHasNonBoolSubForm(terms->sig, tform);
+if(nonbool)
+{
+   ...
+   Error("%s Formula is not Boolean ...", SYNTAX_ERROR, PosRep(...));
+}
```

The offending subformula is *not* printed, only its type — the formula
printers assume a well-typed skeleton and produce truncated garbage on these
inputs (that is how `f @ a` prints as `~(a)`).

The call is unguarded, i.e. it also runs for FOF/TFF/TCF. There it is inert:
in FO an ill-typed atom is already rejected by `TypeInferSort()` while the
term is being built, so the walk always returns NULL. Confirmed empirically
below.

`TFormulaHasNonBoolSubForm()` allocates nothing and has no side effects, so
on every input it does not reject, behaviour is unchanged by construction.

---

## Testing

Built with `./configure --enable-ho && make rebuild`, release and debug
(`DEBUGGER` on, `NODEBUG` off) and once more with `MEMDEBUG`. No new compiler
warnings.

**No real-world problem changes its parse.** Dumped `--print-formulas
--syntax-only` for all 115 `.p`/`.tptp`/`.lop` files in the tree, patched vs
unpatched. Exactly three differ, and all three are deliberately ill-typed bug
reproductions: `bugs/bug005-nonbool-toplevel-lambda-segfault.p` and the two
lambda cases `32`/`90` from the previous PR's corpus. The other 112 are
byte-identical, exit codes included.

**Targeted corpus of 39 hand-written cases** (`bugs/dust/nonbool/`). 19
ill-typed cases that must be rejected, 12 well-typed ones that must keep
working, 8 type errors that must terminate gracefully. Nine SIGSEGVs before,
none after. Before → after (only the rows this check changes):

```
139 -> 3   ^ [X: $o] : ( p & X )        (SIGSEGV -> syntax error)
139 -> 3   ^ [X: $o] : ( p = X )
139 -> 3   f @ a
139 -> 3   p | (f @ a)
139 -> 3   thf(c,conjecture, ^ [X: $o] : ( p & X ))
  0 -> 3   ^ [X: $o] : ( X & p )        (bogus "Unsatisfiable" -> syntax error)
  0 -> 3   ^ [X: $o] : p
  0 -> 3   ^ [X: $i] : ( f @ X )
  0 -> 3   p & (^ [X: $o] : X)
  0 -> 3   ~ (^ [X: $o] : X)
  0 -> 3   ! [X: $o] : (^ [Y: $o] : Y)
  0 -> 3   ^ [X: $i] : (f @ X) = f
  4 -> 3   hq @ (^[X:$o]:(p & a))       (nested below a lambda)
139 -> 3   hq @ (^[X:$o]:(p & (f @ a))) (SIGSEGV, nested below a lambda)
 10 -> 3   hq @ (^[X:$o]:(p & (^[Y:$o]:Y)))
 10 -> 3   hq @ (^[X:$o]:$ite(X,p,(p & a)))
  4 -> 3   p & a                        (late type error -> parse-time)
  4 -> 3   a
  4 -> 3   (g @ a) => a
 10 -> 10  ((&) @ p) @ q                (unchanged)
 10 -> 10  (~) @ p
  0 ->  0  ! [X: $o] : X
  0 ->  0  ! [P: $i > $o] : (P @ a)
 10 -> 10  $ite(p, q, p)
 10 -> 10  ! [X: $i] : ((f @ X) = a)
 10 -> 10  p & (! [X: $i] : ((f @ X) = a))
 10 -> 10  g @ a
 10 -> 10  ! [P: $o] : (P => P)
 10 -> 10  (^ [X: $o] : ( p & X )) @ p
 10 -> 10  hq @ (^[X:$o]:(X & p))       (well-typed, nested)
 10 -> 10  (^[X:$o]:(X & p)) = (^[X:$o]:(p & X))
  4 ->  4  tff(c,axiom, f(a))
```

The remaining rows in the full corpus (the debug-build-only assertion aborts,
and `p @ q` / `(a = a) @ p` / `$true @ p`) are addressed by the two follow-up
PRs, not by this commit.

**No memory leaks.** With `CLB_MEMORY_DEBUG`, `SizeMalloc()ed == SizeFree()ed`
and `New requests == Returned`: 10/10 accepted targeted cases, 33/33 cases of
the previous PR's corpus, and 105/105 real problem files.

**No regressions.** With assertions enabled, all reproductions in `bugs/`
behave as before — no new assertion failures; `bug005` now exits 3 instead of
segfaulting. The bundled first-order problems (`BOO001`, `LUSK3`, `PUZ031`)
still find their proofs.

---

## Note on `^ [X: $i] : (f @ X) = f`

One shape worth flagging, since it interacts with the binder/equation change
that just went in. Written without parentheses, lambda extensionality now
parses as `^ [X: $i] : ((f @ X) = f)` — type `$i > $o` — and is therefore
rejected by this patch. The parenthesised form
`( ^ [X: $i] : (f @ X) ) = f` is unaffected and still works.

That follows from the TPTP BNF (the body of a binder is a
`<thf_unit_formula>`, and `s = t` is one), so rejecting it looks correct to
me — but it does mean a file relying on the old associativity now gets an
error rather than a wrong answer. It appears in none of the problem files in
the tree.
