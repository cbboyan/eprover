% Issue 1 demo: head of phony app invisible in vertical walks.
%
% ax1/ax2: concrete functions f, g applied to a, b
%   -> vertical walk at `a` sees the parent symbol (f or g) -- distinguishable
%
% ax3/ax4: HO free variables X, Y applied to a, b  (phony apps)
%   -> vertical walk at `a` only sees $@_var -- indistinguishable (bug)

thf(type_a, type, a : $i).
thf(type_b, type, b : $i).
thf(type_f, type, f : $i > $i > $o).
thf(type_g, type, g : $i > $i > $o).

thf(ax1, axiom, f @ a @ b).
thf(ax2, axiom, g @ a @ b).
thf(ax3, axiom, ?[X: $i > $i > $o] : (X @ a @ b)).
thf(ax4, axiom, ?[Y: $i > $i > $o] : (Y @ a @ b)).
