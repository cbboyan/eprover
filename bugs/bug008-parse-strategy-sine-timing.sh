# No crash -- the bug is silent, so this compares SinE pruning between two
# equivalent invocations instead of a single reproducer command. See the .md.
#
# bug008-parse-strategy-sine-timing.strategy was generated with:
#   eprover-ho --sine='GSinE(CountFormulas,hypos,5.0,,3,500,1.0)' \
#     --print-strategy bug008-parse-strategy-sine-timing.p
# (header stripped down to the `{ ... }` block).
cd "$(dirname "$0")"

echo "== --sine= given directly on the command line (works) =="
eprover-ho --print-statistics --tstp-format \
  --sine='GSinE(CountFormulas,hypos,5.0,,3,500,1.0)' \
  bug008-parse-strategy-sine-timing.p | grep -i "sine\|relevancy pruning"

echo "== same sine: field loaded via --parse-strategy (broken: 0 removed) =="
eprover-ho --print-statistics --tstp-format \
  --parse-strategy=bug008-parse-strategy-sine-timing.strategy \
  bug008-parse-strategy-sine-timing.p | grep -i "sine\|relevancy pruning"
