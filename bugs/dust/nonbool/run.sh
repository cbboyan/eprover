#!/bin/bash
# Show how eprover-ho reacts to each test case: exit code plus the first
# diagnostic line. Cases 01-15 must be rejected, 20-29 must still be
# accepted, 30-32 must produce a graceful type error (no abort, no segfault).
# Usage: bash dust/nonbool/run.sh [case.p ...]
cd "$(dirname "$0")/cases" || exit 1
E=../../../../PROVER/eprover-ho
files=("$@")
[ ${#files[@]} -eq 0 ] && files=(*.p)
for f in "${files[@]}"; do
   f=$(basename "$f")
   out=$(timeout 60 $E --auto "$f" 2>&1); rc=$?
   # the formula under test is the only non-declaration, non-comment line
   in=$(grep -v '_decl' "$f" | grep -v '^%' | grep -v '^$')
   # a diagnostic, or the SZS status if the file was accepted
   msg=$(echo "$out" | grep -E '^(#|%) (Subformula|Type mismatch|too many)|^eprover' | head -1)
   [ -z "$msg" ] && msg=$(echo "$out" | grep -oE 'SZS status [A-Za-z]+' | head -1)
   # a release-build segfault prints nothing itself, so go by the exit code
   case $rc in
      139) msg="*** SIGSEGV *** $msg" ;;
      134) msg="*** SIGABRT *** $msg" ;;
   esac
   printf '=== %s\n  in : %s\n  rc : %s\n  out: %s\n\n' "$f" "$in" "$rc" "$msg"
done
