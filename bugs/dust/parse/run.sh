#!/bin/bash
# Show how eprover-ho parses each test case (stop right after parsing).
# Usage: bash dust/parse/run.sh [case.p ...]
cd "$(dirname "$0")/cases" || exit 1
E=../../../../PROVER/eprover-ho
files=("$@")
[ ${#files[@]} -eq 0 ] && files=(*.p)
for f in "${files[@]}"; do
   f=$(basename "$f")
   printf '=== %s\n' "$f"
   # input: skip the shared type declarations, show only the real formulae
   grep -v '_decl' "$f" | grep -v '^%' | grep -v '^$' | sed 's/^/  in : /'
   # parsed: likewise drop the $true stubs that type decls turn into
   # the MEMDEBUG footer is checked separately by memcheck.sh; strip it here
   $E --print-formulas --syntax-only "$f" 2>&1 \
      | grep -v '_decl, axiom, ($true))\.' | grep -v '_decl, axiom, $true)\.' \
      | grep -vE '^% (Total|New requests|Returned|SecureRealloc|-----)' \
      | grep -v '^$' \
      | sed 's/^/  out: /'
   echo
done
