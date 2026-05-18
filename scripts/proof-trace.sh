#!/bin/bash
set -euo pipefail

if [ $# -lt 1 ]; then
    echo "Usage: $0 <problem.p>" >&2
    exit 1
fi

problem="$1"
base="${problem%.p}"

eprover -p --proof-log="$base.trace" --proof-log-debug "$problem" > "$base.proof"
