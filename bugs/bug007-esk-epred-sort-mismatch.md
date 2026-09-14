# Bug: sort mismatch between two Skolem symbols during search

**File:** `bugs/bug007-esk-epred-sort-mismatch.p` (unminimized — full
Sledgehammer output, 1663 lines, isa/deeper/B-mesh-th0/train120k corpus,
`HOL-Datatype_Examples/0010_Regex_ACIDZ/prob_00258_009170`)

**Trigger:** `bash bugs/bug007-esk-epred-sort-mismatch.sh` — original,
unminimized strategy (`e-ehoh_best`) that found this. Not bisected yet.

**Status:** OPEN — not investigated, not minimized. Noted for later; see
`bug006-sup-partial-app-type-mismatch.md` for the bisection process to reuse
once this gets picked up.

**Reproduced locally** with `eprover-ho` from `PATH` (`E 3.5.1-ho Countess
Grey`, rev `25808ee34733f8641fc5962acc203d453d9dbf1a`), under `setarch
$(uname -m) -R` (ASLR disabled, per the bug006 finding that this gives
deterministic Skolem/variable numbering) — exit 3, identical message to the
one seen in the original solverpy batch that produced this file.

## Symptom

```
% Error: terms esk5_3 @ epred103_0 @ X249 @ X249: regex_rexp_a and X249: a should have the same sort
eprover: Type error
```

Exit code 3, not a crash.

## How this differs from bug006

Superficially similar (both are a `Type error` abort during
preprocessing/search on a well-typed Sledgehammer HOL input, both exit 3,
neither is a crash) but a **different failure shape**, likely a different
code path:

- The mismatched symbols here, `esk5_3` and `epred103_0`, genuinely are
  E-generated Skolem symbols (`esk*`/`epred*` naming) — unlike bug006, where
  the flagged symbol turned out to be a genuine input constant.
- The message is a **sort mismatch** ("should have the same sort") between
  two Skolem terms, not an arity/curried-application mismatch
  ("expected ... but got ...") like bug006.

So: not investigated further, not assumed to share a root cause with bug006.

## Not yet done

- No minimization — the `.p` is the full 1663-line file as generated, and
  the `.sh` is the full, un-bisected strategy that originally hit this.
- No root-cause analysis of which inference/Skolemization step produces the
  ill-sorted term.
