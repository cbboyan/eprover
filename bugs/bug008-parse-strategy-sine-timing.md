# Bug: `sine:` set via `--parse-strategy`/`--select-strategy` is never applied — loaded after SInE pruning already ran

**Status:** OPEN — root-caused by reading source (`PROVER/eprover.c`,
`CONTROL/cco_sine.c`, `HEURISTICS/che_hcb.c`), not yet reported upstream, not
yet reproduced with a minimized `.p`/`.sh` (no crash to reproduce — the
symptom is silent, observable only by comparing pruning behavior against
what the config declares).

**Found while:** running E's own predefined `--print-strategy` configs as
standalone strategies via `--parse-strategy=<path>`, after noticing
`--auto`/`--auto-schedule` prints two distinct `Configuration:` lines per
problem (one preprocessing-stage pick, one search-stage pick — see below)
and wanting to reproduce individual named configs directly for comparison.

## Symptom

A strategy file loaded via `--parse-strategy=<path>` (or selected via
`--select-strategy=<name>`) that declares a `sine:` field — `"Auto"` or an
explicit `GSinE(...)` filter — runs with **no SInE relevancy pruning applied
at all**, regardless of that field's value. Not a crash, not an error
message; the run just silently proceeds as if `sine: (null)` had been set
(E even prints `"No SInE strategy applied"`, `cco_sine.c:619`, i.e. it
correctly reports what it did — the bug is that what it did doesn't match
the strategy file).

## Root cause

`main()` in `PROVER/eprover.c` runs (line numbers as of the revision in this
clone — check `eprover --version` if line numbers have drifted):

```
606  parse_spec()
624  if (auto_conf || strategy_scheduling): handle_auto_modes_preproc()   -- stage 1 (auto only)
646  ProofStateSinE(proofstate, h_parms->sine)          <- SInE pruning runs HERE
647  ProofStateRelevancyProcess()
681  FormulaSetCNF2()                                    -- clausification
701  ProofStateClausalPreproc()
703  if (...): stage 2 (auto only) -- reclassify, pick search config
777  strategy_io(h_parms, hcb_definitions)                <- --parse-strategy/--select-strategy applied HERE
```

`strategy_io()` (`eprover.c:294-330`) is the only function that reads
`--parse-strategy=<path>` (via `parse_strategy_filename`,
`HeuristicParmsParseInto`) or `--select-strategy=<name>` (via
`select_strategy`, `GetHeuristicWithName`) into `h_parms`, including
`h_parms->sine`. It is called exactly once, at line 777 — **131 lines and a
full clausification pass after** `ProofStateSinE()` at line 646 already ran
and returned.

`h_parms->sine` defaults to `NULL` (`HeuristicParmsInitialize`,
`HEURISTICS/che_hcb.c:172`, `handle->sine = NULL;`) and nothing sets it
earlier for a hand-picked run (no `auto_conf`/`strategy_scheduling`, so the
line-624 block that *would* set it in time never executes). `ProofStateSinE`
treats `NULL` as "nothing to do":

```c
// CONTROL/cco_sine.c:601-611
long ProofStateSinE(ProofState_p state, char* fname)
{
   ...
   if(!fname)
   {
      return 0;
   }
   ...
```

So by the time `strategy_io()` finally writes the file's real `sine:` value
into `h_parms->sine` at line 777, the only call site that ever reads that
field (`cco_sine.c:601`, single call site, confirmed via
`grep -rn ProofStateSinE`) has already executed and exited. The write is a
no-op from that point on — nothing re-reads `h_parms->sine` afterward.

**Confirmed there is no earlier application path**: `--parse-strategy=<path>`
during option parsing (`eprover.c:1367`) only stores the filename string
(`parse_strategy_filename = arg;`); the file is not read until
`strategy_io()`. Same for `--select-strategy=<name>` (`eprover.c:1361`,
`select_strategy = arg;`, applied inside the same `strategy_io()` call).

## Why this looks unintended, not designed

- The `auto` path (`handle_auto_modes_preproc()`, stage 1) goes out of its
  way to set `h_parms->sine` *before* line 646 specifically so
  `ProofStateSinE` sees the real value — proof the ordering constraint is
  understood elsewhere in this same file.
- Both E's own `--print-strategy`/`--parse-strategy` round-trip and
  solverpy's `EProverSid` plugin document the strategy file as fully
  equivalent to the same options given on the command line. A `sine:` field
  that silently does nothing while every other field in the same file works
  breaks that equivalence for one specific field, with no comment or guard
  anywhere flagging it as known/intentional.
- A plain `--sine=...` CLI flag is unaffected — parsed during the initial
  `process_options(argc, argv)`, well before line 646 — so this is narrowly
  about the `--parse-strategy`/`--select-strategy` loading path, not SInE in
  general. That asymmetry (works via CLI flag, silently doesn't via strategy
  file/name) is itself evidence this wasn't a deliberate choice.

## Scope / who this affects

Any strategy run via `--parse-strategy=<path>` or `--select-strategy=<name>`
whose config declares a `sine:` filter. Confirmed affected in practice: a
batch of 9 configs taken from `HEURISTICS/schedule.vars` and run individually
via `--parse-strategy` (3 with an explicit `GSinE(...)` filter, 6 with
`"Auto"`) all ran with zero SInE filtering, contrary to what each config
declares. Not affected: a strategy that passes `--sine=...` directly as a
plain command-line argument instead of through a strategy file — that path
is parsed early enough and works correctly, independently reproduced on a
strategy known to rely on aggressive SInE filtering.

## Not yet done

- Not reported upstream to the E maintainers yet — planned next.
- No minimized `.p`/`.sh` reproducer — the fix (call `strategy_io()`, or at
  least apply `--parse-strategy`/`--select-strategy`'s `sine:` field, before
  line 646) is obvious enough from the source that a repro may not be
  necessary, but one could be built by diffing `GSinE`-filtered axiom counts
  between a `--sine=GSinE(...)` CLI run and an equivalent `--parse-strategy`
  file on the same instance.
- Not checked whether any other `h_parms` field has the same
  loaded-too-late problem — SInE was found because it happens to be the
  only pre-clausification step gated by `h_parms`; other fields consumed
  later in `main()` (after line 777) would not be affected the same way,
  but this wasn't exhaustively checked.
