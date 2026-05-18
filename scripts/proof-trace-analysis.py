#!/usr/bin/env python3
"""
Analyze E Prover proof traces produced by --proof-log.

Uses prooftrace.py for parsing. See that module for trace format details.

Output CSV columns:
  perm_id          stable clause identifier
  lits             literal count of the given clause
  direct_gen       clauses generated directly from this given (GEN + FW)
  bw               backward-simplified clauses produced by this step
  fw               forward-deleted generated clauses (killed before unprocessed)
  net              clauses that entered unprocessed (direct_gen - fw)
  blast_radius     transitive generated count (follows GEN->GIVEN chains)
  is_proof_relevant  1 if this given contributed to the proof (directly or as ancestor)

Usage:
  python3 proof-trace-analysis.py agatha.trace
  python3 proof-trace-analysis.py agatha.trace -o agatha.csv
  python3 proof-trace-analysis.py -          # read from stdin
"""

import sys
import csv
import argparse
import prooftrace


FIELDS = ['perm_id', 'lits', 'direct_gen', 'bw', 'fw', 'net',
          'blast_radius', 'is_proof_relevant']


def main():
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('logfile', nargs='?', default='-',
                        help='proof trace file (default: stdin)')
    parser.add_argument('-o', '--output', default='-',
                        help='output CSV file (default: stdout)')
    args = parser.parse_args()

    infile  = prooftrace.open_trace(args.logfile)
    outfile = sys.stdout if args.output == '-' else open(args.output, 'w', newline='')

    data     = prooftrace.parse_trace(infile)
    relevant = prooftrace.proof_relevant_givens(data)
    cache    = {}

    writer = csv.DictWriter(outfile, fieldnames=FIELDS, delimiter='\t', lineterminator='\n')
    writer.writeheader()
    for pid, step in data.given_steps.items():
        fw   = len(step.fw_ids)
        dgen = len(step.gen_ids) + fw
        bw   = len(step.bw_ids)
        writer.writerow({
            'perm_id':           pid,
            'lits':              step.lits,
            'direct_gen':        dgen,
            'bw':                bw,
            'fw':                fw,
            'net':               dgen - fw,
            'blast_radius':      prooftrace.blast_radius(pid, data, cache),
            'is_proof_relevant': 1 if pid in relevant else 0,
        })

    if args.logfile != '-':
        infile.close()
    if args.output != '-':
        outfile.close()


if __name__ == '__main__':
    main()
