# Working files: binder/equation parse scoping

Test artifacts from the change that makes a binder scope over a complete
literal, so `? [X: $o] : p = X` parses as `? [X: $o] : (p = X)` rather than
`(? [X: $o] : p) = X`. See `PULL-REQUEST.md` for the full write-up.

Code change: `absorb_infix_eqn_tstp_parse()` in `CLAUSES/ccl_tformulae.c`,
called from `quantified_tform_tstp_parse()`.

## Contents

| path | what |
|---|---|
| `PULL-REQUEST.md` | PR description sent upstream (branch `fix-binder-equation-scoping`) |
| `parse/cases/*.p` | 34 targeted cases, one formula per file |
| `parse/run.sh` | prints `in:`/`out:` pairs for the cases |
| `parse/memcheck.sh` | checks the `CLB_MEMORY_DEBUG` footer for leaks |
| `parse/baseline.txt` | case output *before* the change |
| `parse/after.txt` | case output *after* the change |
| `parse/corpus-*.txt` | full `--print-formulas` dumps of all 53 problem files in the tree, patched vs unpatched (regenerable; `v2` = guard removed, debug build) |
| `parse/t0*.p` | first-round exploratory files, superseded by `cases/` |

## Usage

```sh
bash bugs/dust/parse/run.sh                 # all cases
bash bugs/dust/parse/run.sh 01-geoff.p      # one case
diff bugs/dust/parse/baseline.txt bugs/dust/parse/after.txt

# leak check (needs a MEMDEBUG build, see Makefile.vars)
bash bugs/dust/parse/memcheck.sh --auto -- bugs/dust/parse/cases/*.p
```

## Case numbering

* `01`–`07` binder immediately followed by `=`/`!=` — the cases being fixed
* `10`–`16` explicit parenthesisation; `10`/`11` are the reference shapes that
  the fixed cases must now match byte-for-byte. `14`/`15` are the only
  currently-valid inputs whose *meaning* changes.
* `20`–`24` other connectives after a binder — must not change
* `30`–`36` lambda binders, incl. lambda extensionality (`35`/`36`) — must not change
* `40`–`41` `=` followed by a connective — previously syntax errors
* `50`–`51` negation and left-hand context
* `60`–`63` FOF / TFF / TFX / CNF baselines. `62` is the TFX form of the bug.
* `90` repro for `bug005-nonbool-toplevel-lambda-segfault` (pre-existing crash)
