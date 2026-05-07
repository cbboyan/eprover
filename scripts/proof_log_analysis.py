#!/usr/bin/env python3
"""
Analyze E Prover proof logs produced by --proof-log.

Input format:
  GIVEN <id> gen=<n> bw=<n> fw=<n> lits=<n>: (<formula>)
    + GEN <id> lits=<n>
    - BW  <id>
    ! FW  <id> lits=<n>
  PROOF <id1> <id2> ...        (absent for failed/incomplete searches)

Output CSV columns:
  perm_id          stable clause identifier
  lits             literal count of the given clause
  direct_gen       clauses generated directly from this given
  bw               backward-simplified clauses produced by this step
  fw               forward-deleted generated clauses (killed before unprocessed)
  net              clauses that entered unprocessed (direct_gen - fw)
  blast_radius     transitive generated count (follows GEN->GIVEN chains)
  is_proof_relevant  1 if this given contributed to the proof (directly or as ancestor)

Usage:
  python3 proof_log_analysis.py agatha.proof
  python3 proof_log_analysis.py agatha.proof -o agatha.csv
  python3 proof_log_analysis.py -          # read from stdin
"""

import sys
import re
import csv
import argparse
from dataclasses import dataclass, field


@dataclass
class GivenStep:
    perm_id: int
    lits: int
    gen_ids: list = field(default_factory=list)
    bw_ids: list = field(default_factory=list)
    fw_ids: list = field(default_factory=list)


RE_GIVEN = re.compile(r'^GIVEN (\d+) gen=\d+ bw=\d+ fw=\d+ lits=(\d+):')
RE_GEN   = re.compile(r'^\s+\+ GEN (\d+)')
RE_BW    = re.compile(r'^\s+- BW\s+(\d+)')
RE_FW    = re.compile(r'^\s+! FW\s+(\d+)')
RE_PROOF = re.compile(r'^PROOF(.*)')


def parse_log(lines):
    """Parse a single proof log into (given_steps, proof_ids)."""
    given_steps = {}
    current = None

    for raw in lines:
        line = raw.rstrip('\n')

        m = RE_GIVEN.match(line)
        if m:
            current = GivenStep(perm_id=int(m.group(1)), lits=int(m.group(2)))
            given_steps[current.perm_id] = current
            continue

        if current:
            m = RE_GEN.match(line)
            if m:
                current.gen_ids.append(int(m.group(1)))
                continue
            m = RE_BW.match(line)
            if m:
                current.bw_ids.append(int(m.group(1)))
                continue
            m = RE_FW.match(line)
            if m:
                current.fw_ids.append(int(m.group(1)))
                continue

        m = RE_PROOF.match(line)
        if m:
            proof_ids = {int(x) for x in m.group(1).split()}
            return given_steps, proof_ids

    return given_steps, set()  # failed/incomplete search


def proof_relevant_givens(given_steps, proof_ids):
    """
    Return the set of perm_ids of GIVENs that contributed to the proof,
    either directly (perm_id in proof_ids) or as an ancestor
    (generated a clause that is transitively proof-relevant).
    """
    generated_by = {}
    for pid, step in given_steps.items():
        for gid in step.gen_ids:
            generated_by[gid] = pid

    relevant = set()
    queue = list(proof_ids)
    while queue:
        gid = queue.pop()
        if gid in relevant:
            continue
        relevant.add(gid)
        parent = generated_by.get(gid)
        if parent is not None and parent not in relevant:
            queue.append(parent)

    return relevant & given_steps.keys()


def blast_radius(root_id, given_steps, cache=None):
    """
    Transitive count of all clauses spawned from root_id:
    counts direct GEN/BW/FW, plus recursively all clauses spawned by each GEN
    that itself became a GIVEN.
    """
    if cache is None:
        cache = {}
    if root_id in cache:
        return cache[root_id]

    step = given_steps.get(root_id)
    if step is None:
        return 0

    cache[root_id] = 0  # guard against cycles
    total = len(step.gen_ids) + len(step.bw_ids) + len(step.fw_ids)
    for gid in step.gen_ids:
        total += blast_radius(gid, given_steps, cache)
    cache[root_id] = total
    return total


FIELDS = ['perm_id', 'lits', 'direct_gen', 'bw', 'fw', 'net',
          'blast_radius', 'is_proof_relevant']


def main():
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('logfile', nargs='?', default='-',
                        help='proof log file (default: stdin)')
    parser.add_argument('-o', '--output', default='-',
                        help='output CSV file (default: stdout)')
    args = parser.parse_args()

    infile  = sys.stdin  if args.logfile == '-' else open(args.logfile)
    outfile = sys.stdout if args.output  == '-' else open(args.output, 'w', newline='')

    given_steps, proof_ids = parse_log(infile)
    relevant = proof_relevant_givens(given_steps, proof_ids)
    cache = {}

    writer = csv.DictWriter(outfile, fieldnames=FIELDS, delimiter='\t', lineterminator='\n')
    writer.writeheader()
    for pid, step in given_steps.items():
        dgen = len(step.gen_ids)
        bw   = len(step.bw_ids)
        fw   = len(step.fw_ids)
        writer.writerow({
            'perm_id':           pid,
            'lits':              step.lits,
            'direct_gen':        dgen,
            'bw':                bw,
            'fw':                fw,
            'net':               dgen - fw,
            'blast_radius':      blast_radius(pid, given_steps, cache),
            'is_proof_relevant': 1 if pid in relevant else 0,
        })

    if args.logfile != '-':
        infile.close()
    if args.output != '-':
        outfile.close()


if __name__ == '__main__':
    main()
