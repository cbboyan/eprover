# Do not abort on ordinary type errors in `TypeInferSort()`

Found while probing the type checker for `#bug005-nonbool-toplevel-lambda-segfault`.
Independent of that fix — happy to have this merged separately, or dropped.

`TypeInferSort()` reports type errors like this:

```c
fprintf(stderr, COMCHAR" Type mismatch in argument #%d of ", i+1);
...
assert(false);
TI_ERROR("Type error");
```

`TI_ERROR` expands to `AktTokenError(in, msg, false)` or `Error(msg,
SYNTAX_ERROR)`, and both end in `exit()` — so the `assert(false)` on the line
before can only ever convert that clean exit into a SIGABRT. In a debug build:

```tptp
thf(hq_decl,type,hq: ( $o > $o ) > $o).
thf(c,axiom, hq @ p ).
```

```
% Type mismatch in argument #1 of hq @ ($eq @ p @ $true): expected $o > $o but got $o
eprover-ho: cte_typecheck.c:327: TypeInferSort: Assertion `false' failed.
Aborted (core dumped)                 # exit 134
```

after:

```
% Type mismatch in argument #1 of hq @ ($eq @ p @ $true): expected $o > $o but got $o
eprover: Type error                   # exit 3
```

This is user error in an input file, not an invariant violation, so aborting
on it seems wrong — but the asserts do give you a core dump at the point of
the error, which may be the reason they are there. All four are removed here
(`TERMS/cte_typecheck.c` lines 327, 348, 358, 371).

Three of the four are covered by test cases — `30` (argument mismatch, the
non-phony-app branch), `34` (argument mismatch under a variable head, the
phony-app branch) and `33` (`too many arguments supplied for $and`). I could
not construct an input reaching the fourth ("Type mismatch for ... and type
...", a non-arrow type with non-zero arity); the parser appears to reject
those earlier.

---

## Testing

Built with `./configure --enable-ho && make rebuild`, release and debug
(`DEBUGGER` on, `NODEBUG` off) and once more with `MEMDEBUG`. No new compiler
warnings.

Only debug builds (assertions enabled) are affected — in a release build
(`NDEBUG`), `assert()` was already a no-op, so behaviour there is unchanged by
construction.

From the 39-case corpus in `bugs/dust/nonbool/`, the rows this fix changes
(debug build only; release build was already 3 in all of these) — these are
exactly the three assert sites reached by the corpus, verified by hand
against both the unfixed and fixed binary:

```
134 -> 3   hq @ p                              (line 327: arg mismatch, non-phony-app)
134 -> 3   ((&) @ p @ q) @ q                   (line 358: too many arguments supplied for $and)
134 -> 3   ! [F: $i > $i] : ((F @ p) = a)      (line 348: arg mismatch, phony-app)
```

(`(f @ a @ a) = a` is a fourth over-application case in the corpus, but it is
caught earlier by the parser's own argument-count guard and never reaches
`TypeInferSort()` at all — exit 3 before and after, unrelated to this fix.)

**No memory leaks.** With `CLB_MEMORY_DEBUG`, `SizeMalloc()ed == SizeFree()ed`
and `New requests == Returned` for all of the above and for the rest of the
corpus and real problem files — these all exit via `Error()`/`AktTokenError()`
like any other parse error, before and after.

**No regressions.** With assertions enabled, all other reproductions in
`bugs/` behave exactly as before — no new or missing assertion failures
outside the four sites touched here. The bundled first-order problems
(`BOO001`, `LUSK3`, `PUZ031`) still find their proofs.
