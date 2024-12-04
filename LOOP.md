# Proof loop

eprover:main():
   - `FormulaSetCNF()`
   - `ClauseSetPreprocess()` 
   - `cco_proofproc:Saturate()`:
      - `ProcessClause()`: 
          - `control->hcp->hcb_select()`
          - `ForwardContractClause()`
          - `document_processing()`
          - `replacing_inferences()`
          - backward subsumption:
             - `eliminate_backward_rewritten_clauses()`
             - `eliminate_backward_subsumed_clauses()`:
                - `remove_subsumed()`:
                   - `ClauseSetFindFVSubsumedClauses()`
             - `eliminate_unit_simplified_clauses()`
             - `eliminate_context_sr_clauses()`
          - `ccl_clausesset:ClauseSetIndexedInsert()`
          - `generate_new_clauses()`
             - `cco_ho_inferences:ComputeHOInferences()`:
             - `cco_factoring:ComputeAllEqualityFactors()`
             - `cco_eqnresolving:ComputeAllEqnResolvents()`
             - `cco_diseq_decomp:ComputeDisEqDecompositions()`
             - `cco_paramodulation:ComputeAllParamodulants()`
             TODO: get to reporting a newly generated clause





