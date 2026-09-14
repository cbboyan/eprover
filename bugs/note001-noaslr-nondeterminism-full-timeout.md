# Note: same instance/strategy, ASLR off, one run solves fast and one times out completely

**Not a bug report** — no `.p`/`.sh` here, just an observation worth keeping
for later. Found while cross-checking two solverpy batches over the same
TPTP/THF corpus (a larger pool at T5-M4 vs. a smaller strict subset of it at
T10-M4) — both runs launched with `setarch $(uname -m) -R` (ASLR disabled),
per the earlier finding that this makes `eprover-ho`'s internal numbering
reproducible.

## Observation

Comparing the same instance + same strategy across the two batches (`train2k`
run only, since it uses the longer T10 cutoff — T10 should trivially finish
anything T5 finishes):

| strategy | instance | `train120k` run (T5-M4) | `train2k` run (T10-M4) |
|---|---|---|---|
| `ram-2b65a8274485a1ea` | `HOL-Cardinals/0001_Order_Relation_More/prob_00320_009390.p` | Theorem, 0.163s | ResourceOut, 10s (full timeout) |
| `e-pre_casc_10` | `HOL-Analysis/0138_Equivalence_Lebesgue_Henstock_Integration/prob_00195_008151.p` | Theorem, 2.34s | ResourceOut, 10s |
| `e-ehoh_best` | `HOL-Analysis/0103_Starlike/prob_05922_222145.p` | Theorem, 1.80s | ResourceOut, 10s |
| `bls02af` | `HOL-ex/0083_Meson_Test/prob_02282_139845.p` | Theorem, 2.78s | ResourceOut, 10s |

Found among only 4 cases total (out of ~2000 shared instances x several
strategies), so rare, but not marginal when it happens: none of these are
near the 5s or 10s boundary — they solve in under 3s on one run and then
**fail to solve at all within a 10s budget on the same instance, same
strategy, same input file**. This is not the far more common "borderline
near the cutoff, sometimes finishes just under/over" pattern (which shows up
separately as small bidirectional differences when comparing same-cutoff
simulated results) — this is a full non-solve where a fast solve was
otherwise typical.

## Why this is surprising given ASLR is off

The whole point of `setarch $(uname -m) -R` (established in an earlier,
separate T1 comparison) was that disabling ASLR per process makes E's
internal counters (Skolem symbols, free variables) — and
by extension the search — fully reproducible run-to-run, with zero mismatches
across reruns in that earlier T1 experiment. These 4 cases show search
behavior diverging so far between two nominally-identical runs that one
terminates with a proof in ~2s and the other doesn't terminate within 10s —
i.e. either something other than ASLR is still injecting nondeterminism into
the search, or the two runs were not actually as identical as assumed (e.g.
different host load, different date/batch, a config difference not yet
checked).

## Not yet done

- Not confirmed whether these 4 came from the same solverpy batch invocation,
  same day, same host load conditions, or different ones — that's the first
  thing to check before concluding this is genuine search nondeterminism
  rather than an environmental difference between the two runs.
- Not reproduced under a controlled back-to-back rerun (same binary, same
  host, same moment, `setarch -R` both times) to see if the divergence is
  stable/repeatable or itself intermittent.
- No attempt yet to identify what in the search is diverging (would need a
  debug build with derivation tracing, comparing the two runs step by step).
