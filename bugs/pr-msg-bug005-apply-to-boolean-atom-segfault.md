# Guard NULL head type in `applied_tform_tstp_parse()`

Found while probing the type checker for `#bug005-nonbool-toplevel-lambda-segfault`.
Independent of that fix, and independent of `#bug005-typecheck-abort-on-type-error`
— happy to have this merged separately, or dropped. It crashes *inside*
`TFormulaTSTPParse()`, before the non-Boolean check in the other PR ever runs.

Applying anything to a Boolean atom:

```tptp
thf(p_decl,type,p: $o).
thf(q_decl,type,q: $o).
thf(c,axiom, p @ q ).
```

```
eprover-ho: ccl_tformulae.c:683: applied_tform_tstp_parse: Assertion `hd_type' failed.
Segmentation fault (core dumped)      # release build, exit 139
```

`GetHeadType()` returns NULL for a polymorphic head, and an equation is
polymorphic. The head here *is* an equation, because `EncodePredicateAsEqn()`
has just rewritten the Boolean atom `p` into `$eq(p,$true)`. The NULL then
reaches `TypeGetMaxArity(hd_type)` and is dereferenced. Same for
`(a = a) @ p` and `$true @ p`.

Such a head is saturated by construction, so applying anything to it is
simply an error — which the function already has a message for, two lines
further down:

```c
 const Type_p hd_type = GetHeadType(terms->sig, head);
-assert(hd_type);
+if(!hd_type)
+{
+   AktTokenError(in, " Too many arguments applied to the term", false);
+}
 const int max_args = TypeGetMaxArity(hd_type);
```

The guard fires only where the old code dereferenced NULL, so nothing that
previously worked can be affected.

---

## Testing

Built with `./configure --enable-ho && make rebuild`, release and debug
(`DEBUGGER` on, `NODEBUG` off) and once more with `MEMDEBUG`. No new compiler
warnings.

Verified all three reproductions by hand, before and after:

```
139 -> 3   p @ q
139 -> 3   (a = a) @ p
139 -> 3   $true @ p
```

**No memory leaks.** With `CLB_MEMORY_DEBUG`, `SizeMalloc()ed == SizeFree()ed`
and `New requests == Returned` for all three — they exit via
`AktTokenError()` like any other parse error.

**No regressions.** With assertions enabled, all other reproductions in
`bugs/` behave exactly as before — no new or missing assertion failures
outside the one site touched here. The bundled first-order problems
(`BOO001`, `LUSK3`, `PUZ031`) still find their proofs.
