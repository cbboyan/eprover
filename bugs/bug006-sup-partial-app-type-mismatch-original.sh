eprover-ho -s -p -R --print-statistics --proof-statistics --tstp-format \
  --memory-limit=2048 --soft-cpu-limit=5 --cpu-limit=15 \
  --delete-bad-limit=2000000000 --condense --strong-rw-inst --no-eq-unfolding \
  --sos-uses-input-types \
  --literal-selection-strategy=SelectMaxLComplexAPPNTNp \
  --term-ordering=KBO6 --order-precedence-generation=invfreqconjmax \
  --order-weight-generation=precrank10 --definitional-cnf=4 \
  --forward-demod-level=1 --order-constant-weight=1 --simul-paramod \
  --destructive-er --destructive-er-aggressive --strong-destructive-er \
  --neg-ext=all --pos-ext=all --ext-sup-max-depth=0 \
  --lift-lambdas=false --local-rw=true --fool-unroll=false \
  --satcheck=ConjMinMinFreq --satcheck-proc-interval=5000 \
  --define-heuristic='(2*ConjectureRelativeSymbolWeight(PreferGround,0.5,100,100,100,100,1.5,1.5,1),6*ConjectureRelativeSymbolWeight(ByDerivationDepth,0.1,100,100,100,100,1.5,1.5,1.5),1*Refinedweight(PreferGoals,3,2,2,1.5,2),2*ConjectureRelativeSymbolWeight(PreferNonGoals,0.5,100,100,100,100,1.5,1.5,1),2*ConjectureRelativeSymbolWeight(PreferGround,0.5,100,100,100,100,1.5,1.5,1),1*FIFOWeight(ConstPrio))' \
  bug006-sup-partial-app-type-mismatch.p
