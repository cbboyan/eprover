#!/bin/bash
# Run eprover-ho (MEMDEBUG build) on each file and verify allocated == freed.
# Usage: bash dust/parse/memcheck.sh <eprover-args...> -- <files...>
cd "$(dirname "$0")/../../.." || exit 1
E=./PROVER/eprover-ho

args=(); files=()
seen=0
for a in "$@"; do
   if [ "$a" = "--" ]; then seen=1; continue; fi
   if [ $seen -eq 0 ]; then args+=("$a"); else files+=("$a"); fi
done

fail=0; ok=0; nofoot=0
for f in "${files[@]}"; do
   out=$(timeout 60 $E "${args[@]}" "$f" 2>&1); rc=$?
   mal=$(echo "$out" | grep -oP 'SizeMalloc\(\)ed memory: \K[0-9]+ Bytes \([0-9]+ requests\)')
   fre=$(echo "$out" | grep -oP 'SizeFree\(\)ed   memory: \K[0-9]+ Bytes \([0-9]+ requests\)')
   new=$(echo "$out" | grep -oP 'New requests: *\K[0-9]+')
   ret=$(echo "$out" | grep -oP 'Returned: *\K[0-9]+')
   if [ -z "$mal" ] || [ -z "$new" ]; then
      printf 'NOFOOTER  rc=%-4s %s\n' "$rc" "$f"; nofoot=$((nofoot+1)); continue
   fi
   if [ "$mal" = "$fre" ] && [ "$new" = "$ret" ]; then
      ok=$((ok+1))
   else
      printf 'LEAK      rc=%-4s %s\n' "$rc" "$f"
      printf '            malloc=%s\n            free  =%s\n            new=%s returned=%s\n' \
             "$mal" "$fre" "$new" "$ret"
      fail=$((fail+1))
   fi
done
echo "--- clean=$ok  leaking=$fail  nofooter=$nofoot"
[ $fail -eq 0 ]
