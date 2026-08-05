%------ Simple binder-then-equation, well-typed either way ------
thf(a_decl,type,a: $i).
thf(f_decl,type,f: $i > $i).
thf(t1,axiom, ! [X: $i] : X = a ).
thf(t2,axiom, ! [X: $i] : (f @ X) = a ).
thf(t3,axiom, ? [X: $i] : X = a ).
thf(t4,axiom, ! [X: $i] : X != a ).
