tff(a_decl,type, a: $i).
tff(f_decl,type, f: $i > $i).
tff(pp_decl,type, pp: $i > $o).
tff(q_decl,type, q: $o).
tff(c1,axiom, ! [X: $i] : X = a ).
tff(c2,axiom, ! [X: $i] : f(X) = a ).
tff(c3,axiom, ! [X: $i] : pp(X) & q ).
tff(c4,axiom, ? [X: $i] : X != a ).
