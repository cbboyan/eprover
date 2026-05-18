#!/usr/bin/env python3
"""
Render an E Prover proof trace as a GraphViz dot graph.

Node types:
  INIT   — rectangle, colored by role (axiom/conjecture/…)
  CNF    — box, one per clause produced from an INIT formula
  GIVEN  — ellipse, clauses selected during saturation
  GEN    — diamond, clauses generated but never selected as given
  FW     — box (grey), generated and immediately forward-deleted (--fw only)

Edge types:
  INIT → CNF         solid black  (clause produced from formula)
  CNF  → GIVEN       dotted grey  (COPY relationship)
  GIVEN → GEN/GIVEN  solid black  (inference produced this clause)
  GIVEN → BW-target  dashed red   (this given backward-simplified target)
  GIVEN → FW         dotted grey  (generated and immediately killed)

Proof-relevant nodes are filled palegreen.

Usage:
  python3 proof-trace-graph.py agatha.trace | dot -Tsvg -o agatha.svg
  python3 proof-trace-graph.py agatha.trace -o agatha.dot
  python3 proof-trace-graph.py agatha.trace --labels | dot -Tpdf -o agatha.pdf
  python3 proof-trace-graph.py -   # read from stdin
"""

import argparse
import re
import sys
import prooftrace


ROLE_COLOR = {
    'axiom':             'lightblue',
    'conjecture':        'lightyellow',
    'negated_conjecture':'lightsalmon',
    'hypothesis':        'lightcyan',
    'plain':             '#eeeeee',
}
PROOF_FILL   = 'palegreen'
DEFAULT_FILL = 'white'
FW_FILL      = '#dddddd'
BW_FILL      = '#ffdddd'


def esc(s):
    return s.replace('\\', '\\\\').replace('"', '\\"')


def clause_label(pid, data, max_len=0):
    if max_len:
        formula = data.formulas.get(pid, '')
        if formula:
            if len(formula) > max_len:
                formula = formula[:max_len] + '…'
            return f'{pid}\\n{esc(formula)}'
    return str(pid)


def node_id(pid):
    return f'p{pid}'


def init_node_id(name):
    return 'init_' + re.sub(r'[^a-zA-Z0-9_]', '_', name)


def main():
    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('tracefile', nargs='?', default='-',
                        help='proof trace file (default: stdin)')
    parser.add_argument('-o', '--output', default='-',
                        help='output dot file (default: stdout)')
    parser.add_argument('--labels', metavar='N', type=int, nargs='?',
                        const=40, default=0,
                        help='show formula labels on clause nodes, truncated to N chars (default 40)')
    parser.add_argument('--bw', action='store_true',
                        help='show backward-simplification edges')
    parser.add_argument('--fw', action='store_true',
                        help='show forward-deleted clause nodes and edges')
    parser.add_argument('--no-copy', action='store_true',
                        help='omit CNF nodes and COPY edges (draw INIT → GIVEN directly)')
    args = parser.parse_args()

    infile  = prooftrace.open_trace(args.tracefile)
    outfile = sys.stdout if args.output == '-' else open(args.output, 'w')

    data     = prooftrace.parse_trace(infile)
    relevant = prooftrace.proof_relevant_givens(data)

    # perm_ids that appear as proof-line ids or their copy targets
    proof_direct = set(data.proof_ids)
    for orig, copy in data.copy_map.items():
        if orig in data.proof_ids:
            proof_direct.add(copy)

    def fill(pid):
        return PROOF_FILL if (pid in relevant or pid in proof_direct) else DEFAULT_FILL

    # Collect all GEN-only perm_ids (generated but never selected as given)
    gen_only = set()
    for step in data.given_steps.values():
        for gid in step.gen_ids:
            if gid not in data.given_steps:
                gen_only.add(gid)

    # Collect all CNF perm_ids (from INIT lines)
    cnf_pids = set()
    for entry in data.init_entries:
        for cnf in entry.cnf_entries:
            cnf_pids.add(cnf.perm_id)

    label_len = args.labels

    w = outfile.write
    w('digraph proof_trace {\n')
    w('  rankdir=TB;\n')
    w('  node [fontsize=10, fontname="Courier"];\n')
    w('  edge [fontsize=9];\n\n')

    # ── INIT nodes ────────────────────────────────────────────────────────────
    w('  // INIT nodes\n')
    for entry in data.init_entries:
        nid   = init_node_id(entry.name)
        color = ROLE_COLOR.get(entry.role, '#eeeeee')
        w(f'  {nid} [shape=box, style=filled, fillcolor="{color}", '
          f'label="{esc(entry.name)}\\n({entry.role})"];\n')

    # ── CNF nodes and INIT→CNF edges ──────────────────────────────────────────
    if not args.no_copy:
        w('\n  // CNF nodes\n')
        for entry in data.init_entries:
            for cnf in entry.cnf_entries:
                pid  = cnf.perm_id
                lbl  = clause_label(pid, data, label_len)
                clr  = fill(pid)
                w(f'  {node_id(pid)} [shape=box, style=filled, fillcolor="{clr}", label="{lbl}"];\n')

        w('\n  // INIT → CNF edges\n')
        for entry in data.init_entries:
            nid = init_node_id(entry.name)
            for cnf in entry.cnf_entries:
                w(f'  {nid} -> {node_id(cnf.perm_id)};\n')

        w('\n  // COPY edges (CNF → GIVEN)\n')
        for orig, copy in data.copy_map.items():
            w(f'  {node_id(orig)} -> {node_id(copy)} [style=dotted, color=grey];\n')
    else:
        # draw INIT → GIVEN directly using the copy_map
        copy_inv = {v: k for k, v in data.copy_map.items()}
        init_for_cnf = {}
        for entry in data.init_entries:
            for cnf in entry.cnf_entries:
                init_for_cnf[cnf.perm_id] = init_node_id(entry.name)

        w('\n  // INIT → GIVEN edges (copy edges folded in)\n')
        for given_pid, cnf_pid in copy_inv.items():
            src = init_for_cnf.get(cnf_pid)
            if src:
                w(f'  {src} -> {node_id(given_pid)};\n')

    # ── GIVEN nodes ───────────────────────────────────────────────────────────
    w('\n  // GIVEN nodes\n')
    for pid, step in data.given_steps.items():
        lbl = clause_label(pid, data, label_len)
        clr = fill(pid)
        w(f'  {node_id(pid)} [shape=ellipse, style=filled, fillcolor="{clr}", label="{lbl}"];\n')

    # ── GEN-only nodes ────────────────────────────────────────────────────────
    w('\n  // GEN-only nodes\n')
    for pid in gen_only:
        lbl = clause_label(pid, data, label_len)
        clr = fill(pid)
        w(f'  {node_id(pid)} [shape=diamond, style=filled, fillcolor="{clr}", label="{lbl}"];\n')

    # ── FW nodes ──────────────────────────────────────────────────────────────
    if args.fw:
        fw_pids = set()
        for step in data.given_steps.values():
            fw_pids.update(step.fw_ids)
        fw_pids -= data.given_steps.keys()  # skip any that became GIVEN
        w('\n  // FW nodes (forward-deleted)\n')
        for pid in fw_pids:
            lbl = clause_label(pid, data, label_len)
            w(f'  {node_id(pid)} [shape=box, style=filled, fillcolor="{FW_FILL}", '
              f'color=grey, label="{lbl}"];\n')

    # ── Inference edges: GIVEN → GEN ──────────────────────────────────────────
    w('\n  // Inference edges\n')
    for pid, step in data.given_steps.items():
        for gid in step.gen_ids:
            w(f'  {node_id(pid)} -> {node_id(gid)};\n')
        if args.bw:
            for bid in step.bw_ids:
                w(f'  {node_id(pid)} -> {node_id(bid)} '
                  f'[style=dashed, color=red, label="bw"];\n')
        if args.fw:
            for fid in step.fw_ids:
                w(f'  {node_id(pid)} -> {node_id(fid)} '
                  f'[style=dotted, color=grey, label="fw"];\n')

    w('}\n')

    if args.tracefile != '-':
        infile.close()
    if args.output != '-':
        outfile.close()


if __name__ == '__main__':
    main()
