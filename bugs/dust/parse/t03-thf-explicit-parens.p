%------ Explicit parenthesisation: MUST keep its current meaning ------
thf(a_decl,type,a: $i).
thf(q_decl,type,q: $o).
thf(pp_decl,type,pp: $i > $o).
% inner-scoped equation, already unambiguous
thf(t1,axiom, ! [X: $i] : ( X = a ) ).
% outer equation between a quantified formula and q: must stay as is
thf(t2,axiom, ( ! [X: $i] : (pp @ X) ) = q ).
% same, disequality
thf(t3,axiom, ( ? [X: $i] : (pp @ X) ) != q ).
