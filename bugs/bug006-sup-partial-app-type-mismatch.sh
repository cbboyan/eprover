# Minimized reproducer (bisected down from the original strategy that found
# this, kept in bug006-sup-partial-app-type-mismatch-original.sh). Run with
# ASLR disabled for reproducible Skolem/variable numbering -- see the .md.
setarch $(uname -m) -R eprover-ho -s \
  --memory-limit=2048 --soft-cpu-limit=5 --cpu-limit=15 \
  --strong-rw-inst --no-eq-unfolding \
  --order-precedence-generation=invfreqconjmax \
  --order-weight-generation=precrank10 \
  --lift-lambdas=false --fool-unroll=false \
  --define-heuristic='(6*ConjectureRelativeSymbolWeight(ByDerivationDepth,0.1,100,100,100,100,1.5,1.5,1.5),2*ConjectureRelativeSymbolWeight(PreferNonGoals,0.5,100,100,100,100,1.5,1.5,1))' \
  bug006-sup-partial-app-type-mismatch.p
