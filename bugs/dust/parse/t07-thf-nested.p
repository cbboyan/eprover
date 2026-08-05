%------ Nested / stacked binders and trickier shapes ------
thf(a_decl,type,a: $i).
thf(f_decl,type,f: $i > $i > $i).
thf(q_decl,type,q: $o).
% two binders, then =
thf(t1,axiom, ! [X: $i] : ? [Y: $i] : (f @ X @ Y) = a ).
% binder, then =, then a connective: how far does the absorption go?
thf(t2,axiom, ! [X: $i] : X = a & q ).
% negated binder
thf(t3,axiom, ~ ( ! [X: $i] : X = a ) ).
% binder under a connective on the left
thf(t4,axiom, q & ( ! [X: $i] : X = a ) ).
