%------ Lambda binder followed by = ------
thf(a_decl,type,a: $i).
thf(f_decl,type,f: $i > $i).
thf(g_decl,type,g: $i > $i).
thf(hh_decl,type,hh: ( $i > $o ) > $o).
thf(t1,axiom, hh @ ( ^ [X: $i] : (f @ X) = (g @ X) ) ).
thf(t2,axiom, hh @ ( ^ [X: $i] : ( (f @ X) = (g @ X) ) ) ).
