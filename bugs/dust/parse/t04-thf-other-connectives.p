%------ Other connectives after a binder: MUST NOT change ------
%------ TPTP: binder binds the smallest unitary formula, so these
%------ all associate OUTSIDE the quantifier.
thf(q_decl,type,q: $o).
thf(pp_decl,type,pp: $i > $o).
thf(t1,axiom, ! [X: $i] : (pp @ X) & q ).
thf(t2,axiom, ! [X: $i] : (pp @ X) | q ).
thf(t3,axiom, ! [X: $i] : (pp @ X) => q ).
thf(t4,axiom, ! [X: $i] : (pp @ X) <=> q ).
thf(t5,axiom, ? [X: $i] : (pp @ X) & q ).
