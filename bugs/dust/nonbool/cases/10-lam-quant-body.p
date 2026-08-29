% top level is $o, but body of ! is $o > $o
thf(p_decl,type,p: $o).
thf(q_decl,type,q: $o).
thf(a_decl,type,a: $i).
thf(f_decl,type,f: $i > $i).
thf(g_decl,type,g: $i > $o).
thf(hq_decl,type,hq: ( $o > $o ) > $o).
thf(c,axiom, ! [X: $o] : (^ [Y: $o] : Y) ).
