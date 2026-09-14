# Original, unminimized strategy that found this (e-ehoh_best). Not bisected
# yet -- see the .md.
setarch $(uname -m) -R eprover-ho -s -p -R --print-statistics --proof-statistics --tstp-format \
  --memory-limit=2048 --soft-cpu-limit=5 --cpu-limit=15 \
  -tKBO6 --order-weight-generation=invfreqrank --order-precedence-generation=invfreq \
  --strong-rw-inst --order-constant-weight=1 --no-eq-unfolding --presat-simplify \
  --literal-selection-strategy=SelectComplexExceptUniqMaxHorn --simul-paramod \
  --forward-context-sr --forward-demod-level=1 --destructive-er-aggressive \
  --strong-destructive-er --satcheck-proc-interval=5000 --satcheck=ConjMinMinFreq \
  --delete-bad-limit=2000000000 --neg-ext=all --pos-ext=all --ext-sup-max-depth=1 \
  -H'(1*ConjectureRelativeSymbolWeight(SimulateSOS,0.5,100,100,100,100,1.5,1.5,1),4*ConjectureRelativeSymbolWeight(ConstPrio,0.1,100,100,100,100,1.5,1.5,1.5),1*FIFOWeight(PreferProcessed),1*ConjectureRelativeSymbolWeight(PreferNonGoals,0.5,100,100,100,100,1.5,1.5,1),4*Refinedweight(SimulateSOS,3,2,2,1.5,2))' \
  bug007-esk-epred-sort-mismatch.p
