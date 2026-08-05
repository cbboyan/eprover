thf(a_decl,type,a: $i).
thf(f_decl,type,f: $i > $i).
thf(pp_decl,type,pp: $i > $o).
thf(q_decl,type,q: $o).
thf(c,axiom, ( ! [X: $i] : (pp @ X) ) = ( ? [Y: $i] : (pp @ Y) ) ).
