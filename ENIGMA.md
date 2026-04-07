# ENIGMA — Development Overview

ENIGMA (Efficient learNing-based Inference Guidance MAchine) is a machine-learning-based
clause selection system for the E prover. It learns which clauses are useful for finding
proofs from previous proof searches and applies that knowledge to guide future searches.

This document summarizes the development arc across 8 papers (2017–2022).

---

## Core Idea

The saturation-based proof search in E repeatedly selects a "given clause" from the
*Unprocessed* queue and processes it against the *Processed* set. The choice of which
clause to select next is the main lever for guiding the search. ENIGMA trains a binary
classifier to predict whether a clause is *proof-relevant* (positive) or not (negative):

- **Positives**: clauses that appeared in the final proof
- **Negatives**: other clauses that were selected but did not contribute to the proof

The trained model is plugged into E as a custom weight function. Two integration modes:

- **S⊙M** (solo): the model selects all given clauses
- **S⊕M** (cooperative): 50/50 split — half clauses selected by E's built-in heuristics,
  half by the model

The evaluation loop: prove problems → extract training data → train model → prove with
model → iterate.

---

## Paper 1 — ENIGMA v1 (CICM 2017)

**Paper**: "ENIGMA: Efficient Learner-Independent Guidance for Saturation-based ATPs"
**File**: `enigma/978-3-319-62075-6_20.pdf`

### Features

Clauses are represented as **term-tree walks of length 3** (directed paths of length 3
through the term tree):

- Variables → `⊛` (all vars unified to one anonymous symbol)
- Skolem functions → `⊙`
- Positive literals marked with `⊕`, negative with `⊖`
- Paths are directed: down-left, down-right, up from each node

Each walk becomes a feature string; the clause feature vector is a bag-of-walks (sparse
integer counts).

### Classifier

**LIBLINEAR** (L2-regularized SVM): fast, linear model. O(n) evaluation per clause in
the number of non-zero features.

### Results

Evaluated on **200 AIM problems** (CASC 2016). S⊕M cooperative mode outperforms E
alone by ~10% (more problems solved within the time limit). Solo mode S⊙M is less robust
but occasionally very strong.

---

## Paper 2 — ENIGMA v2 (CICM 2018)

**Paper**: "Improving ENIGMA-style Clause Selection while Learning From History"
**File**: `enigma/978-3-319-96812-4_11.pdf`

### Extended Features

The feature representation is significantly extended:

- **Vertical walks**: same as v1 (length-3 directed paths)
- **Horizontal walks**: paths through sibling arguments (lateral structure)
- **Symbol statistics**: raw symbol occurrence counts
- **Length features**: clause length, literal count
- **Conjecture features**: the feature vector is doubled — first half is the clause
  features, second half is conjecture features (same representation). This allows the
  model to relate clause content to the goal.

### Accuracy-Balancing Boosting

A new iterative re-training procedure: after each training round, collect the
misclassified positive examples and add them (with higher weight) to the next training
batch. This corrects for the severe class imbalance (proofs touch a tiny fraction of
all selected clauses) and improves recall on proof-relevant clauses.

### Results

Evaluated on **MPTP2078** (2078 Mizar problems). Extended features + boosting give
substantial improvement over v1. Conjecture features are the single biggest gain.

---

## Paper 3 — ENIGMAWatch (TABLEAUX 2019)

**Paper**: "ENIGMAWatch: ProofWatch Meets ENIGMA"
**File**: `enigma/978-3-030-29026-9_21.pdf`

### ProofWatch

ProofWatch is a separate heuristic that tracks the *completion ratio* of multiple
candidate proofs Π simultaneously. Each candidate proof Π is a partial proof that
E is trying to complete. The ratio φ_Π(t) ∈ [0,1] measures how close the current
proof state is to completing Π at time t.

### Integration

ENIGMAWatch adds the ProofWatch completion vector **φ_Π** as extra input features to
ENIGMA. The combined input is:

```
[clause features | conjecture features | φ_Π]
```

The φ_Π vector gives the model proof-state context: not just "does this clause look
like proof clauses from history?" but "given how close we are to each candidate proof,
is this clause likely useful now?"

### Results

ENIGMAWatch improves over plain ENIGMA on MPTP2078. The proof-state signal is
complementary to the static clause features.

---

## Paper 4 — Hammering Mizar (ITP 2019)

**Paper**: "Hammering Mizar"
**File**: `enigma/LIPIcs.ITP.2019.34.pdf`

### Scale

A large-scale application: **57880 Mizar problems** from the Mizar Mathematical Library.
The full MPTP corpus, not just the 2078-subset used in earlier work.

### XGBoost

Switches from LIBLINEAR to **XGBoost** (gradient-boosted decision trees):

- Better accuracy on the same feature vectors as v2
- Naturally handles sparse features; no linear separability assumption
- More expensive to evaluate than LIBLINEAR but still fast enough for practical use

### Evaluation

The paper benchmarks Hammers (ATP-based premise selection + proof attempt) on the full
Mizar corpus. ENIGMA-guided E is one component. The combination of premise selection
(choosing which axioms to include) with ENIGMA guidance (choosing which clauses to
process) is shown to be complementary.

---

## Paper 5 — ENIGMA-NG (CADE 2019)

**Paper**: "ENIGMA-NG: Efficient Neural and Gradient-Boosted Inference Guidance..."
**File**: `enigma/978-3-030-29436-6_12.pdf`

### Feature Hashing (NG = Next Generation)

To handle **large corpora** (>10⁶ distinct features from big Mizar libraries), feature
strings are mapped to fixed-size buckets via the **sdbm hash function**. This reduces
the feature dimension from millions to a fixed width (e.g., 2²⁰ = 1M buckets) with
acceptable collision rates. Allows scaling to corpora where the full feature vocabulary
would be too large.

### Gradient-Boosted Decision Trees

**LightGBM** replaces XGBoost as the primary GBDT implementation:

- Faster training on sparse data
- Lower memory footprint
- Native support for the hashed feature format

LightGBM is integrated into E as a C library (`CONTRIB/libs/lib_lightgbm.a`,
`CONTRIB/lightgbm.h`) — clause weights are computed by calling into LightGBM from
E's weight function interface.

### Recursive Neural Networks

**ENIGMA-NG** also introduces the first neural approach: **LSTM-based RNN** applied
to clause term sequences (DFS traversal of the term tree). The RNN automatically learns
feature representations instead of using hand-crafted walks.

Integration: the RNN model runs as a **TorchEval** weight function via PyTorch C++ API.
Term sharing in E is exploited: identical subterms are evaluated once and their embedding
cached, making RNN evaluation much cheaper than naive re-computation.

### Results

On **57897 Mizar problems** (full MPTP corpus). LightGBM outperforms LIBLINEAR and XGBoost.
The RNN model achieves similar or slightly better accuracy but is slower without caching.
With caching, the RNN is competitive in wall-clock time.

---

## Paper 6 — ENIGMA Anonymous (IJCAR 2020)

**Paper**: "ENIGMA Anonymous: Symbol-Independent Inference Guiding Machine..."
**File**: `enigma/978-3-030-51054-1_29-2.pdf`

### The Symbol-Independence Problem

A model trained on proofs of Mizar problems knows the symbols of the Mizar library. It
cannot generalize to problems with completely different symbol vocabularies (e.g., a new
theory). The feature-hashing approach of ENIGMA-NG partially mitigates this but does not
truly achieve symbol independence.

### Anonymization

Two anonymization schemes replace symbol names with **arity-only tokens**:

- **fn/pm**: function symbol of arity n becomes `fn`; predicate of arity m becomes `pm`
- Symbol order within arguments is preserved (positional information retained)

This collapses all symbols of the same arity into one feature, making the representation
symbol-independent. Transfer learning becomes possible: train on one library, evaluate
on another.

### Graph Neural Networks

**GNN** (message-passing neural network on a hypergraph):

- **Nodes**: clause nodes, symbol nodes, subterm nodes
- **Edges**: containment (clause→literal→subterm), symbol application (symbol→args)
- **Message passing**: R rounds of neighborhood aggregation; each node's embedding
  is updated by aggregating embeddings of its neighbors

Key properties of the GNN approach:
- **Symbol-independent** by construction (symbol embeddings are learned but not
  required to match between train and test)
- **Context-based**: multiple query clauses are evaluated jointly with *context clauses*
  (the processed set). The GNN receives the full proof state, not just one clause in
  isolation. This is in contrast to all earlier ENIGMA variants which evaluate each
  clause independently.

### Results

On MPTP. The GNN's context-awareness gives a substantial advantage over GBDT: it can
assess whether a clause is *novel relative to the current processed set*, not just whether
it resembles proof clauses in the abstract.

---

## Paper 7 — Fast & Slow + Parental Guidance (FroCoS 2021)

**Paper**: "Fast and Slow Enigmas and Parental Guidance"
**File**: `enigma/978-3-030-86205-3_10.pdf`

### GPU Server

Running GNN evaluations inside E's inner loop is expensive. The paper introduces a
**persistent GPU server**: a multi-threaded Python/PyTorch process that accepts
evaluation requests from E clients via Unix socket. Multiple E instances (parallel
portfolio) share one GPU server.

Communication protocol: E serializes clause features to a socket; server runs a
batched GNN forward pass on GPU; returns scores. Amortizes Python/GPU startup cost
across thousands of evaluations.

### 2-Phase ENIGMA (Fast & Slow)

GNNs are expensive even with the GPU server. The solution: **two-phase evaluation**:

1. **Fast phase**: cheap GBDT (LightGBM) pre-filters clauses — rejects clearly
   bad clauses immediately (score below threshold)
2. **Slow phase**: remaining clauses are sent to the GNN for precise scoring

The GBDT runs in-process (microseconds per clause); the GNN handles only the ~top-k
candidates per cycle. This gives most of the GNN's accuracy at a fraction of its cost.

### Parental Guidance

Instead of (or in addition to) evaluating a new clause, evaluate its **parents** — the
clauses from which it was derived. The intuition: if both parents were proof-relevant,
the child is more likely to be proof-relevant.

Two parental feature combination strategies:

- **P_cat** (concatenate): parent feature vectors are concatenated → larger input
- **P_fuse** (merge/add): parent feature vectors are element-wise summed → same size

**P_cat > P_fuse** in experiments. Parental guidance is especially useful because it
can be computed *before* the clause is fully normalized, giving early rejection of
clauses that will be expensive to normalize.

A **frozen set** tracks clauses rejected by parental guidance — they are not retried
even if later evidence would suggest reconsidering.

### Results

On MPTP and ablation experiments. The 2-phase approach nearly matches pure GNN accuracy
while being 5–10× faster. Parental guidance adds additional gains, especially for deep
proof searches.

---

## Paper 8 — Isabelle ENIGMA (ITP 2022)

**Paper**: "Isabelle ENIGMA"
**File**: `enigma/LIPIcs.ITP.2022.16.pdf`

### Isabelle / Sledgehammer

Sledgehammer is Isabelle's proof search component: it selects premises from the Isabelle
library and calls external ATPs (including E). The corpus is **~276363 FOF/TFF problems**
from **179 Isabelle sessions** — much larger and more varied than the Mizar corpus.

Problems are in **typed first-order form (TFF)** (not untyped FOF as in earlier ENIGMA
work), introducing an additional challenge: type annotations on terms.

### ENIGMA for Isabelle

The paper adapts ENIGMA to the Isabelle/Sledgehammer workflow:

- GNN-based model (from ENIGMA Anonymous) applied to TFF problems
- **Premise selection** component: GNN also scores candidate premises before the ATP
  call, reducing the number of axioms passed to E
- Type information is incorporated into the feature representation (type symbols
  are part of the term structure fed to the GNN)

### Closed-Loop Training

A key challenge: Isabelle problems are not pre-solved. The paper develops a
**bootstrapping** procedure:

1. Run E (without ENIGMA) on all problems → collect proofs
2. Train ENIGMA on those proofs
3. Run E+ENIGMA → collect new proofs (including problems E alone couldn't solve)
4. Augment training data → retrain → iterate

Each iteration solves more problems; training data grows richer. After several
iterations the model achieves robust coverage of the Isabelle library.

### Results

On 276363 Isabelle/Sledgehammer problems. ENIGMA-guided E solves substantially more
problems than E alone (roughly +15–25% depending on time limit). The combined
premise selection + clause selection guidance gives additional gains over clause
selection alone.

---

## Development Timeline

```
2017  ENIGMA v1        LIBLINEAR, length-3 term-tree walks
2018  ENIGMA v2        Extended features (horiz+vert+stats+conjecture), accuracy-balancing boosting
2019  ENIGMAWatch      ProofWatch completion ratios as proof-state context
2019  Hammering Mizar  XGBoost, full MPTP corpus (57k problems)
2019  ENIGMA-NG        Feature hashing (sdbm), LightGBM, LSTM/RNN + TorchEval, term-sharing cache
2020  ENIGMA Anon.     Symbol-independent anonymization (fn/pm), GNNs, context-based evaluation
2021  Fast & Slow      GPU server, 2-phase (GBDT→GNN), Parental Guidance (P_cat/P_fuse)
2022  Isabelle ENIGMA  TFF, Sledgehammer integration, GNN premise selection, bootstrapped training
```

---

## Implementation in This Fork

The `enigma` branch adds ~7800 lines of new C code across ~57 files. The key components
are described below.

### Branch Structure

The enigma branch diverges from master in:

| Area | Files | Purpose |
|------|-------|---------|
| `HEURISTICS/` | `che_enigmaticdata.[ch]`, `che_enigmaticvectors.[ch]`, `che_enigmatictensors.[ch]`, `che_enigmaticweightlgb.[ch]`, `che_enigmaticweightxgb.[ch]`, `che_enigmaticweighttfs.[ch]` | Feature extraction, vector filling, classifier weight functions |
| `CLAUSES/ccl_proofstate.[ch]` | +142 lines | Training data extraction; `ProofStateTrain`, `ProofStateEnigmaticInit` |
| `CONTROL/cco_proofproc.c` | +179 lines | Parental guidance filter in `generate_new_clauses`; frozen-set logic |
| `SIMPLE_APPS/` | `enigmatic-features.c`, `enigmatic-tensors.c` | Standalone CLI tools for feature extraction and tensor serialization |
| `CONTRIB/` | `libs/lib_lightgbm.a`, `lightgbm.h` | LightGBM C shared library |

Bug fixes from `master` (bug001–bug004) are merged into `enigma` periodically; the
merge performed at the start of this session brought them in.

---

### Feature Extraction (`che_enigmaticvectors.c`, `che_enigmaticdata.[ch]`)

#### Feature specifier

Features are controlled by a **specifier string** passed as `--enigmatic-sel-features` (for
selection) or `--enigmatic-gen-features` (for generation filtering). Example:

```
C(l,p,x,s,r,v,h,c,d,a):G:T:P
```

Blocks separated by `:`:

| Block | Meaning |
|-------|---------|
| `C(...)` | Clause being evaluated |
| `G` | Goal (conjecture/negated conjecture) clauses |
| `T` | Theory (axiom) clauses |
| `P` | E's problem-level feature vector (43 values) |
| `W` | ProofWatch completion ratios (ENIGMAWatch) |

Block arguments select feature sub-types:

| Arg | Feature | Size |
|-----|---------|------|
| `l` | 25 length statistics (lits, depth, width, var/pred/func counts, …) | 25 |
| `p` | 22 E priority function values | 22 |
| `x` | Variable occurrence histograms | 3×count |
| `s` | Symbol occurrence histograms | 6×count |
| `r` | Arity histograms | 4×count |
| `v[l=3;b=1024]` | Vertical (depth-first path) walks, length `l`, hashed to `b` buckets | `b` |
| `h[b=1024]` | Horizontal (sibling) walks | `b` |
| `c[b=1024]` | Symbol count hashes | `b` |
| `d[b=1024]` | Symbol depth hashes | `b` |
| `a` | Anonymize symbols (arity-only: `fn`, `pm`) | — |

Parent clause vectors (for Parental Guidance) are separate blocks:
`mother` (1st parent), `father` (2nd parent), `spirit` (accumulated remaining parents).

#### Symbol representation (`symbol_string`)

`symbol_string` in `che_enigmaticvectors.c:181` converts a term node to a feature string
with several modes controlled by `EnigmaticParams_p`:

- **Normal**: use the symbol name from the signature directly
- **Skolem** (`esk*`, `epred*`): replace with `?f<arity>` / `?p<arity>`
- **Anonymous** (`params->anonymous`): replace any non-internal symbol with `f<arity>` / `p<arity>`
- **Typed** (`params->use_types`): append `:type` suffix to every symbol string (supports TFF; enables Isabelle ENIGMA)
- **Free variable**: `*` (anonymous) or `*:type` (typed)
- **De Bruijn variable**: `^` (or `^:type`) — DB vars use f_code=0 via the `FCODE` macro
  (`#define FCODE(term) (TermIsDBVar(term) ? 0 : (term)->f_code)`) so they hash identically

#### Walk computation (`update_verts`, `update_horiz`)

Both functions build up feature strings by traversing the path stack `info->path`
(maintained via `PStackPushP` / `PStackPop` during `update_term`):

- **`update_verts`** (`che_enigmaticvectors.c:342`): reads the last `length_vert` entries
  from the path stack. Each entry is a `Term_p`; its symbol string is joined with `|`
  separators. The concatenated string is hashed via `sdbm` and mapped into `[0, base_vert)`.
- **`update_horiz`** (`che_enigmaticvectors.c:384`): records the current node and all its
  argument symbols as a `.`-separated string, hashed similarly.

The sdbm hash (`hash_sdbm`) is applied character-by-character as the string is built,
avoiding temporary string allocation in the hot path.

#### Parent features (`enigmatic_parents`)

`enigmatic_parents` in `che_enigmaticvectors.c:695` extracts parent clauses from
`clause->derivation` (via `EnigmaticExtractParents`), then fills:

- `vector->mother` ← 1st parent (using dedicated `EnigmaticInfo` var offset)
- `vector->father` ← 2nd parent
- `vector->spirit` ← accumulated average of 3rd+ parents

This corresponds to the **P_cat** strategy from the FroCoS 2021 paper (concatenated
parent vectors in the same feature vector, at separate offsets).

#### Data structures

```c
EnigmaticParams_p   // one set of feature parameters (for one block)
EnigmaticFeatures_p // parsed specifier: offsets and params for all blocks
EnigmaticClause_p   // feature values for one clause
EnigmaticVector_p   // full vector: clause + mother + father + spirit + goal + theory + problem
EnigmaticInfo_p     // traversal context: path stack, sig, bank, var offset, symbol cache
EnigmaticModel_p    // model: filename, handle, features spec, vector, info
EnigmaticSetting_p  // per-ProofState state: sel vector, gen vector, filter model
```

The entire vector is filled into a flat `float[]` via `EnigmaticVectorFill(vector, fill_fun, data)`.
`fill_fun` writes `(index, value)` pairs — the same callback is used for both LightGBM
(CSR sparse format) and the GNN server (dense tensor format).

---

### LightGBM Weight Function (`che_enigmaticweightlgb.[ch]`)

**File**: `HEURISTICS/che_enigmaticweightlgb.c`

The weight function is registered in E's `WFCB` framework and called once per clause
selection cycle. The call chain:

```
EnigmaticWeightLgbCompute(data, clause)        // WFCB entry point
  EnigmaticPredictLgb(clause, local, model1)
    EnigmaticPredict(clause, model, data, fill_fun, predict_fun)
      EnigmaticVectorReset(model->vector)       // zero all feature arrays
      EnigmaticClause(model->vector, clause, model->info)  // fill features
      EnigmaticVectorFill(model->vector, fill_fun, data)   // serialize to LGB CSR
      predict_fun(data, model)                  // LGBM_BoosterPredictForCSRSingleRow
  EnigmaticWeight(pred, weight_type, threshold) // map score → weight
```

The **two-model** setup implements the **Fast & Slow** (2-phase) approach from FroCoS 2021:

```c
pred = EnigmaticPredictLgb(clause, local, model1);  // fast LightGBM
res  = EnigmaticWeight(pred, model1->weight_type, model1->threshold);
if (local->model2 && res == EW_POS) {               // passed fast filter
    pred = EnigmaticPredictLgb(clause, local, model2);  // slow LightGBM (or GNN score)
    res  = EnigmaticWeight(pred, model2->weight_type, model2->threshold);
} else if (local->model2) {
    res = EW_WORST;   // rejected by fast filter
}
```

`EnigmaticWeight` converts a raw model prediction to a clause priority weight:

- `weight_type=0`: continuous `1 + (EW_POS − pred) × EW_NEG`
- `weight_type=1`: binary threshold — `EW_POS` (1.0) if above threshold, `EW_NEG` (10.0) if below
- `weight_type=2`: sigmoid-like `2 − pred/(1+|pred|)`

`EW_POS=1.0`, `EW_NEG=10.0`, `EW_WORST=100.0`. Lower weight = higher priority in E's
clause queue.

---

### GNN / GPU Server Weight Function (`che_enigmaticweighttfs.[ch]`, `che_enigmatictensors.[ch]`)

**Files**: `HEURISTICS/che_enigmaticweighttfs.c`, `HEURISTICS/che_enigmatictensors.c`

This implements the **context-aware GNN** evaluation from ENIGMA Anonymous / FroCoS 2021.
E connects to an external GPU server (Python/PyTorch) via TCP socket.

#### Context management

`EnigmaticWeightTfsParam_p` maintains:

```c
PStack_p    conj_clauses;       // conjecture clauses (fixed for the whole run)
PStack_p    ctx_fixed;          // fixed context (top-N processed clauses by weight)
FloatTree_p ctx_variable;       // sliding context (tracked by weight; evicted on overflow)
long        context_size;       // total context budget
```

Before each batch evaluation, `recompute_context` rebuilds the context:
1. All conjecture clauses go in first (always in context)
2. Fixed processed clauses next
3. Sliding context fills remaining budget, evicting worst-weighted clauses

`EnigmaticTensorsUpdateClause` builds the **hypergraph** representation for each clause
in `che_enigmatictensors.c`: clause nodes, symbol nodes, subterm nodes, and edges between
them are accumulated in `EnigmaticTensorsCell`.

#### Evaluation flow

```
EnigmaticWeightTfsCompute(data, clause)
  // uses clause->ext_weight set by server from previous batch
  weight = EnigmaticWeight(clause->ext_weight, weight_type, threshold)

// separately, every N clauses:
  EnigmaticSocketSend(sock, tensors)    // send batch hypergraph
  scores = EnigmaticSocketRecv(sock, n) // receive GNN scores
  // scores are written into clause->ext_weight for each context clause
```

`clause->ext_weight` is E's field for externally-assigned weights; it is pre-populated by
the server response and read on the next call to `TfsCompute`. This decouples computation
from the per-clause inner loop.

If a LightGBM model (`local->lgb`) is also configured, it acts as the **fast pre-filter**:
`tfs_eval_lgb` runs LGB on all clauses in the unprocessed set, assigning `EW_WORST` to
clearly bad ones so they never get sent to the GNN server.

---

### Training Data Extraction (`ccl_proofstate.c`)

After each proof attempt, `ProofStateTrain` (`ccl_proofstate.c:631`) scans all clause
sets and labels each clause:

- **Positive** (`+1`): clause has property `CPIsSOS` set — it participated in the proof
- **Negative** (`-0`): clause was selected but is not in the proof

`ProofStatePickTrainingExamples` (`ccl_proofstate.c:599`) iterates over:
`ax_archive`, `processed_pos_rules`, `processed_pos_eqns`, `processed_neg_units`,
`processed_non_units`, and `archive`.

Each example is printed as a feature vector via the same `PStackClausePrint` →
`EnigmaticClause` → `EnigmaticVectorFill` → `PrintKeyVal` pipeline used during
inference, so training features and inference features are guaranteed to be identical.

The `--print-training-examples` / `--print-neg-training` / `--print-parents` CLI flags
control which labels and which parent features are included in the output.

---

### Parental Guidance Filter (`cco_proofproc.c`)

The parental guidance filter (FroCoS 2021) is implemented in `generate_new_clauses`
(`cco_proofproc.c:700`). After inference generates a new clause into `state->tmp_store`,
the filter runs immediately before the clause enters the main unprocessed set:

```c
double pred = EnigmaticPredictLgb(handle, filter, filter->model1);
if (pred <= filter->model1->threshold) {
    ClauseSetInsert(state->frozen_store, handle);  // frozen: not retried
    state->frozen_count++;
    continue;
}
// otherwise: ClauseSetInsert(state->unprocessed, handle)
```

`state->frozen_store` holds rejected clauses permanently (unless `--revive-children` is
set, which re-evaluates frozen clauses when new processed clauses are added). Statistics
are printed by `ProofStateStatisticsPrint`:

```
# ...frozen by parental guidance       : N
```

The filter operates on **parent features**: `filter->model1->vector` has `mother`/`father`/
`spirit` blocks but no `clause` block — the new clause itself is not yet normalized, so
only parent features are available. This matches the P_cat design from FroCoS 2021.

---

### Standalone Tools (`SIMPLE_APPS/`)

#### `enigmatic-features`

Reads TPTP input files and prints feature vectors for each clause/formula, supporting all
specifier syntax. Handles `cnf`/`fof`/`tff`/`thf` input. Used to build training datasets
outside the prover loop:

```sh
enigmatic-features --features="C(l,p,x,s,r,v,h,c,d,a)" --problem=problem.p clauses.p
```

Output: one line per clause, sparse `key:value` format, optionally prefixed with
`+1`/`-0` class labels.

**THF/HO input:** type annotation formulas (`thf(name, type, ...)`) are automatically
skipped — they update the signature but produce no feature vector. Non-clause `thf`
formulas are converted to a unit clause by wrapping the formula as a positive predicate
atom (`formula = $true`). By default, THF formulas are parsed raw (De Bruijn conversion
only, no beta/eta normalization). Pass `--normalize-ho` to apply `LambdaNormalizeDB`
after De Bruijn conversion.

#### `enigmatic-tensors`

Serializes clause sets as hypergraph tensors for the GNN server (for batch offline
evaluation or debugging the tensor encoding). **FOF only** — does not support TFF or THF
input.

---

### CLI Options (enigma branch additions to `e_options.h`)

| Option | Purpose |
|--------|---------|
| `--enigmatic-sel-features=SPEC` | Feature specifier for given-clause selection weight function |
| `--enigmatic-gen-features=SPEC` | Feature specifier for generation filtering (parental guidance) |
| `--enigmatic-gen-model=DIR` | LightGBM model directory for generation filtering |
| `--enigmatic-gen-threshold=VAL` | Threshold for generation filter (default 0.5) |
| `--enigmatic-output-map=FILE` | Write feature index map to FILE |
| `--enigmatic-output-buckets=FILE` | Write sdbm hash bucket statistics to FILE |

The `EnigmaticWeightLgb` and `EnigmaticWeightTfs` weight functions are invoked from
heuristic strings like `-H'(1*EnigmaticWeightLgb(...))'` — same as any other E weight
function.

---

### HO Extensions in Enigma Branch

Several commits extend ENIGMA features to handle higher-order (HO) terms:

- `FCODE(term)` macro (`che_enigmaticvectors.c:35`): `TermIsDBVar(term) ? 0 : term->f_code`.
  De Bruijn variables have positive `f_code` equal to their DB index, but should all hash
  as the same symbol (`^`) regardless of index. Mapping them to 0 achieves this.
- `symbol_string` handles DB vars specially: returns `^` (or `^:type` in typed mode).
  This is how DB-bound variables become symbol-independent in HO features.
- The `use_types` parameter (triggered by the `a` modifier + `use_types` flag) appends
  type strings via `DStrAppendType`, enabling typed features for TFF/THF problems.
- HO-specific terms (lambda nodes `f_code=18/19`, phony apps `f_code=17`) are treated as
  regular symbols in the feature paths — their `f_code` maps to the internal `$db_lam` /
  `$@_var` names in the signature, which the feature code handles via `symbol_internal`.
- Commit `ho enigma: disable term cache` disables E's term weight cache for HO problems
  (the cache assumes FO-style weight stability which HO rewriting violates).

The `--enable-ho` / `ENABLE_LFHO` build flag is required to run ENIGMA on HO problems.
