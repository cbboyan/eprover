# Parse `[Binder X]: s = t` as `[Binder X]: (s = t)`

Implements the parser change Geoff asked for: a binder should scope over a
complete literal, so `? [X: $o] : p = X` is `? [X: $o] : (p = X)` rather than
`(? [X: $o] : p) = X`.

Two commits:

1. `CLAUSES/ccl_tformulae.c` — the parser change itself.
2. `CLAUSES/ccl_derivation.c` — an unrelated one-line-ish fix; the file does
   not compile with assertions enabled. Included because it blocks any debug
   build (details at the end).

---

## 1. The parser change

### What E did before

E bound the smallest possible term/formula, so the equation was formed *outside*
the binder. Geoff's example:

```tptp
thf(p_decl,type,p: $o).
thf(an,axiom,? [X: $o] : p = X ).
```

```
eprover: Formula has free variables (check parentheses and quantifier precedence)
```

E parsed `(? [X:$o] : p) = X`, which leaves `X` outside its binder. Depending on
the shape you got a free-variable error, a type error, or a silently wrong
parse:

| input | old parse |
|---|---|
| `? [X:$o] : p = X` | error: free variables |
| `! [X:$i] : X = a` | `(![X:$i]:X) <=> a` |
| `! [X:$i] : X != a` | `(![X:$i]:X) <~> a` |
| `! [X:$i] : ?[Y:$i] : (f@X) = (g@Y)` | error: free variables |
| `hh @ ( ^[X:$i] : (f@X) = (g@X) )` | `$eq @ ($named_lam @ X @ (f@X)) @ (g@X)` → type error |

Note the `<=>`: since both sides sit at formula level, the `=` was additionally
rewritten into an equivalence between the quantified formula and the RHS.

### What it does now

The binder body is parsed as a complete literal:

| input | new parse |
|---|---|
| `? [X:$o] : p = X` | `?[X1:$o]:((p)<=>(X1))` |
| `! [X:$i] : X = a` | `![X1:$i]:((X1)=(a))` |
| `! [X:$i] : X != a` | `![X1:$i]:((X1)!=(a))` |
| `! [X,Y] : (f@X) = (g@Y)` | `![X1,X2]:((f@X1)=(g@X2))` |
| `hh @ ( ^[X:$i] : (f@X) = (g@X) )` | `hh @ (^[X1:$i]:((f@X1)=(g@X1)))` |

Each of these now produces output **byte-identical** to the corresponding
hand-parenthesised form, which is the obvious correctness criterion.

### Justification

The TPTP BNF already says this. The body of a quantification is a
`<thf_unit_formula>`, and one of its alternatives is

```
<thf_defined_infix> ::= <thf_unitary_term> <defined_infix_pred> <thf_unitary_term>
```

i.e. `s = t` is itself a unit formula. So the equation belongs inside the
binder, while everything else still associates outside it.

### Implementation

A helper, plus one call at the single point where a binder body is parsed:

```c
static TFormula_p absorb_infix_eqn_tstp_parse(Scanner_p in, TB_p terms,
                                              TFormula_p lhs)
{
   Sig_p      sig = terms->sig;
   TFormula_p rhs;
   FunCode    op;

   if(!TestInpTok(in, EqualSign|NegEqualSign))
   {
      return lhs;
   }
   op  = tptp_operator_parse(sig, in);
   rhs = literal_tform_tstp_parse(in, terms);

   /* Same encoding as in TFormulaTSTPParse(): an equation between two
      formulas is an equivalence, a disequation an exclusive or. */
   if(lhs->type == sig->type_bank->bool_type)
   {
      op = (op == sig->eqn_code) ? sig->equiv_code : sig->xor_code;
   }
   return TFormulaFCodeAlloc(terms, op, lhs, rhs);
}
```

called from `quantified_tform_tstp_parse()`:

```c
 rest = literal_tform_tstp_parse(in, terms);
+rest = absorb_infix_eqn_tstp_parse(in, terms, rest);
```

Three properties keep the blast radius small:

* It fires **only** on `=`/`!=`, so `&`, `|`, `=>`, `<=>` after a binder still
  associate outside it, exactly as before.
* It absorbs **exactly one** infix step, so `![X]: X = a & q` is
  `(![X]:(X=a)) & q` — the `&` still binds outside.
* It runs **only** for a binder body, never for a parenthesised formula, so
  `( ![X:$i] : (pp@X) ) = q` is untouched and still an equivalence.

It covers `!`, `?` and `^` at once, since all three dispatch through
`quantified_tform_tstp_parse()`.

### All TPTP syntaxes, not just THF

The call is deliberately unguarded. In most FO cases it is inert, because
`EqnParseInfix()` has already consumed the equation while parsing the body —
which is why plain FOF was always correct here.

But `EqnParseInfix()` (`CLAUSES/ccl_eqn.c:480`) takes a shortcut when the LHS is
a predicate with fixed type:

```c
if(problemType == PROBLEM_FO && !TermIsFreeVar(lterm) &&
   SigIsPredicate(bank->sig, lterm->f_code) &&
   SigIsFixedType(bank->sig, lterm->f_code))
{
   rterm = bank->true_term; /* Non-Equational literal */
}
```

It builds a non-equational literal and never looks for `=`. So the TFX form of
Geoff's example had the same defect:

```tptp
tff(p_decl,type, p: $o).
tff(c1,axiom, ? [X: $o] : p = X ).      % was: error: free variables
                                        % now: ?[X1:$o]:((p<=>X1))
```

In *valid* FOF a predicate can never be followed by `=`, so FOF is unaffected
either way — confirmed empirically below.

---

## ⚠️ One behaviour change on currently-valid input

Worth calling out explicitly. Where the binder body is Boolean and the bound
variable does **not** occur on the right, the old parse was well-typed and
meaningful, and this changes its meaning:

```tptp
thf(c,axiom, ! [X:$i] : (pp @ X) = q ).
  before:  (![X1:$i]:(pp @ X1)) <=> q
  after:   ![X1:$i]: ((pp @ X1) <=> q)
```

These are not equivalent. Same for `!=`. This is the intended consequence of
the requested change and is what the BNF mandates, but it does mean the change
is not purely "error → accepted". Any existing file relying on the old
associativity changes meaning silently.

It appears in none of the problem files tested (see below) — presumably
because such files have been TPTP4X-ified to insert the parens.

---

## 2. `CLAUSES/ccl_derivation.c` — does not compile with assertions on

Unrelated to the parser, but it blocks any debug build. With `NDEBUG` off:

```
ccl_derivation.c:2689:18: error: 'opids' undeclared (first use in this function)
ccl_derivation.c:2689:51: error: 'optheory' undeclared (first use in this function)
ccl_derivation.c:2690:51: error: 'opstatus' undeclared (first use in this function)
```

`DerivationComputeAndPrint()` opens with

```c
assert(sizeof(opids) / sizeof(char*) == sizeof(optheory) / sizeof(char*));
assert(sizeof(opids) / sizeof(char*) == sizeof(opstatus) / sizeof(char*));
```

but the parallel arrays `opids[]`, `optheory[]`, `opstatus[]` were retired in
favour of the `opinfo[]` struct array and are commented out (lines 101, 166,
231 of the same file). The assertions only ever compiled because `NDEBUG`
removes them before the undeclared names are looked up.

They are commented out here, with a note. The invariant they checked — that the
three fields stay in step — is now guaranteed by construction, since they are
fields of one struct.

(Happy to instead delete them outright, or reinstate an equivalent check
over `opinfo[]`, whichever you prefer.)

---

## Testing

Built with `./configure --enable-ho && make rebuild`, both release and debug
(`MEMDEBUG` + `DEBUGGER` on, `NODEBUG` off).

**No real-world problem changes its parse.** Dumped `--print-formulas
--syntax-only` for all 53 `.p`/`.tptp` files in the tree (`bugs/`,
`EXAMPLE_PROBLEMS/LFHOL`, `EXAMPLE_PROBLEMS/TPTP`, `EXAMPLE_PROBLEMS/SMOKETEST`,
…) with and without the patch. Output is **byte-identical**, exit codes
included. Checked separately for the guarded and unguarded versions of the
call.

**Targeted corpus of 34 hand-written cases.** Covers each binder (`!`, `?`,
`^`), `=` and `!=`, multiple variables, nested binders, explicit inner and
outer parenthesisation, every other connective after a binder, lambda
extensionality, and FOF/TFF/TFX/CNF baselines. Every case either changes
exactly as intended or is untouched; the fixed cases match their
hand-parenthesised twins byte-for-byte.

Unchanged, verified: explicit outer parens (`(![X]:(pp@X)) = q`), both sides
quantified, `&`/`|`/`=>`/`<=>` after a binder, lambda extensionality
(`(^[X]:f@X) = (^[X]:g@X)` and `(^[X]:f@X) = g`, i.e. the input to
`LambdaToForall`), and all FOF/TFF/CNF cases.

**Newly accepted** (previously syntax errors, now parsed per the BNF):

```
! [X:$i] : X = a & q     ->  (![X1:$i]:((X1)=(a))) & (q)
! [X:$i] : X = a => q    ->  (![X1:$i]:((X1)=(a))) => (q)
```

**No memory leaks.** With `CLB_MEMORY_DEBUG`, checked that
`SizeMalloc()ed == SizeFree()ed` and `New requests == Returned` in the footer:
33/33 targeted cases and 50/50 real problems clean. (The remainder are
pre-existing failures that produce no footer, identical with and without the
patch.)

**No regressions.** With assertions enabled: all bug reproductions in `bugs/`
behave as before (no new assertion failures), and the bundled first-order
problems (`BOO001`, `LUSK3`, `SET103`, `PUZ031`, `NUM030`, `RNG019`, `SET366`)
all still find proofs.

---

## Related pre-existing crash (not fixed here)

Found while testing; **pre-existing and independent** — verified by rebuilding
without the patch:

```tptp
thf(p_decl,type,p: $o).
thf(c,axiom, ^ [X: $o] : ( p & X ) ).
```

```
eprover-ho: ccl_tcnf.c:2040: TFormulaNNF: Assertion `form && terms' failed.
```

(SIGSEGV in a release build.) The axiom is ill-typed — a `thf` formula must
have type `$o`, but this has `$o > $o` — and E does not reject it; the
un-applied lambda reaches clausification and `TFormulaNNF` recurses into a
NULL. Argument order matters: `( X & p )` is fine, `( p & X )` crashes.

The parse of that input is unaffected by this PR (the body is parenthesised).
But the PR does make the crash reachable from *new* input: `^ [X:$o] : p = X`
previously failed with "free variables" and now parses to the same shape. So
it may be worth fixing alongside — presumably by type-checking that an
annotated formula has type `$o` right after parsing.

Happy to send that as a separate PR if useful.
