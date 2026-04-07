#!/bin/bash
# Usage: encode.sh [-f features] problem.p
# Produces problem.in (vectors), problem.map, problem.buckets

FEATURES="C(v,h)"

while getopts "f:" opt; do
   case $opt in
      f) FEATURES="$OPTARG" ;;
      *) echo "Usage: $0 [-f features] problem.p" >&2; exit 1 ;;
   esac
done
shift $((OPTIND-1))

if [ $# -ne 1 ]; then
   echo "Usage: $0 [-f features] problem.p" >&2
   exit 1
fi

BASE="${1%.p}"
../SIMPLE_APPS/enigmatic-features -f "$FEATURES" -m "$BASE.map" -b "$BASE.buckets" -o "$BASE.in" --print-clause "$1"
