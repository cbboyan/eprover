%------ FOF / TFF baselines: these should be unaffected by any THF hack ------
fof(f1,axiom, ! [X] : X = a ).
fof(f2,axiom, ! [X] : f(X) = a ).
fof(f3,axiom, ! [X] : p(X) & q ).
fof(f4,axiom, ? [X] : X != a ).
tff(t_decl,type, b: $i).
tff(pq_decl,type, r: $i > $o).
tff(s_decl,type, s: $o).
tff(t1,axiom, ! [X: $i] : X = b ).
tff(t2,axiom, ! [X: $i] : r(X) & s ).
