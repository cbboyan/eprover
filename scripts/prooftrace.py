"""
Shared parsing for E Prover proof traces produced by --proof-log.

Trace format:
  INIT <role> <name>
    + CNF <id> lits=<n>[: (<formula>)]
  COPY <orig>: <copy>
  GIVEN <id> [<clause_id>] gen=<n> bw=<n> fw=<n> lits=<n>[: (<formula>)]
    + GEN <id> lits=<n>[: (<formula>)]
    - BW  <id> lits=<n>[: (<formula>)]
    ! FW  <id> lits=<n>[: (<formula>)]
  PROOF <id1> <id2> ...
"""

import re
import sys
from dataclasses import dataclass, field


@dataclass
class CnfEntry:
    perm_id: int
    lits: int
    formula: str = ''


@dataclass
class InitEntry:
    role: str
    name: str
    cnf_entries: list = field(default_factory=list)


@dataclass
class GivenStep:
    perm_id: int
    lits: int
    formula: str = ''
    clause_id: str = ''
    gen_ids: list = field(default_factory=list)
    bw_ids: list = field(default_factory=list)
    fw_ids: list = field(default_factory=list)


@dataclass
class TraceData:
    init_entries: list   # [InitEntry]
    copy_map:     dict   # orig_perm_id -> copy_perm_id
    given_steps:  dict   # perm_id -> GivenStep
    proof_ids:    set    # perm_ids from PROOF line
    formulas:     dict   # perm_id -> formula string (from any source)
    lits:         dict   # perm_id -> lits count


RE_INIT  = re.compile(r'^INIT (\S+) (\S+)')
RE_CNF   = re.compile(r'^\s+\+ CNF (\d+) lits=(\d+)(?:: (.+))?')
RE_COPY  = re.compile(r'^COPY (\d+): (\d+)')
RE_GIVEN = re.compile(r'^GIVEN (\d+)(?:\s+(\S+))? gen=\d+ bw=\d+ fw=\d+ lits=(\d+)(?:: (.+))?')
RE_GEN   = re.compile(r'^\s+\+ GEN (\d+) lits=(\d+)(?:: (.+))?')
RE_BW    = re.compile(r'^\s+- BW\s+(\d+) lits=(\d+)(?:: (.+))?')
RE_FW    = re.compile(r'^\s+! FW\s+(\d+) lits=(\d+)(?:: (.+))?')
RE_PROOF = re.compile(r'^PROOF(.*)')


def parse_trace(lines):
    """Parse a proof trace into a TraceData."""
    init_entries = []
    copy_map     = {}
    given_steps  = {}
    proof_ids    = set()
    formulas     = {}
    lits         = {}

    current_init  = None
    current_given = None

    def record(pid, l, formula):
        lits[pid] = l
        if formula:
            formulas[pid] = formula

    for raw in lines:
        line = raw.rstrip('\n')

        m = RE_INIT.match(line)
        if m:
            current_given = None
            current_init = InitEntry(role=m.group(1), name=m.group(2))
            init_entries.append(current_init)
            continue

        if current_init:
            m = RE_CNF.match(line)
            if m:
                pid, l, formula = int(m.group(1)), int(m.group(2)), m.group(3) or ''
                entry = CnfEntry(perm_id=pid, lits=l, formula=formula)
                current_init.cnf_entries.append(entry)
                record(pid, l, formula)
                continue

        m = RE_COPY.match(line)
        if m:
            current_init = None
            copy_map[int(m.group(1))] = int(m.group(2))
            continue

        m = RE_GIVEN.match(line)
        if m:
            current_init = None
            pid, cid = int(m.group(1)), m.group(2) or ''
            l, formula = int(m.group(3)), m.group(4) or ''
            current_given = GivenStep(perm_id=pid, lits=l, formula=formula, clause_id=cid)
            given_steps[pid] = current_given
            record(pid, l, formula)
            continue

        if current_given:
            m = RE_GEN.match(line)
            if m:
                pid, l, formula = int(m.group(1)), int(m.group(2)), m.group(3) or ''
                current_given.gen_ids.append(pid)
                record(pid, l, formula)
                continue
            m = RE_BW.match(line)
            if m:
                pid, l, formula = int(m.group(1)), int(m.group(2)), m.group(3) or ''
                current_given.bw_ids.append(pid)
                record(pid, l, formula)
                continue
            m = RE_FW.match(line)
            if m:
                pid, l, formula = int(m.group(1)), int(m.group(2)), m.group(3) or ''
                current_given.fw_ids.append(pid)
                record(pid, l, formula)
                continue

        m = RE_PROOF.match(line)
        if m:
            proof_ids = {int(x) for x in m.group(1).split() if x}
            break

    return TraceData(init_entries=init_entries, copy_map=copy_map,
                     given_steps=given_steps, proof_ids=proof_ids,
                     formulas=formulas, lits=lits)


def proof_relevant_givens(data):
    """
    Return the set of perm_ids of GIVENs that contributed to the proof,
    either directly or as an ancestor (generated a transitively relevant clause).
    Resolves COPY: PROOF ids may reference original CNF perm_ids that were
    COPYed before being processed as GIVEN.
    """
    generated_by = {}
    for pid, step in data.given_steps.items():
        for gid in step.gen_ids:
            generated_by[gid] = pid

    seed = set(data.proof_ids)
    for orig, copy in data.copy_map.items():
        if orig in data.proof_ids:
            seed.add(copy)

    relevant = set()
    queue = list(seed)
    while queue:
        gid = queue.pop()
        if gid in relevant:
            continue
        relevant.add(gid)
        parent = generated_by.get(gid)
        if parent is not None and parent not in relevant:
            queue.append(parent)

    return relevant & data.given_steps.keys()


def blast_radius(root_id, data, cache=None):
    """
    Transitive count of all clauses spawned (GEN/BW/FW) from root_id,
    recursing into GEN children that themselves became GIVEN.
    """
    if cache is None:
        cache = {}
    if root_id in cache:
        return cache[root_id]
    step = data.given_steps.get(root_id)
    if step is None:
        return 0
    cache[root_id] = 0
    total = len(step.gen_ids) + len(step.bw_ids) + len(step.fw_ids)
    for gid in step.gen_ids:
        total += blast_radius(gid, data, cache)
    cache[root_id] = total
    return total


def open_trace(path):
    return sys.stdin if path == '-' else open(path)
