ENIGMA
------

* `HEURISTICS/che_enigmaticvectors.c`
* `HEURISTICS/che_enigmaticdata.c`

Printing
--------

* `CLAUSES/ccl_clauses.c` / `ClausePrint`
* `CLAUSES/ccl_eqnlist.c` / `EqnListPrint`
* `TERMS/cte_termbanks.c` / `TBPrintTerm`, `TBPrintTermFull`
* `TERMS/cte_termfunc.c` / `TermPrintFO`, `TermPrintHO`, `do_ho_print`

* training samples: `ProofStateTrain` in `CLAUSES/ccl_proofstate.c`

Options
-------

* option enum: `OptionCodes` in `PROVER/e_options.h`
* processing: `process_options` in `PROVER/eprover.c`

