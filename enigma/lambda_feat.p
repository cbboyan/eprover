thf(type_a, type, a : $i).
thf(type_p, type, p : $i > $o).
thf(type_q, type, q : $i > $o).

% lambda at top-level: equality between two lambda expressions
thf(ax1, axiom, (^[X: $i]: (p @ X)) = (^[Y: $i]: (q @ Y))).

% lambda as arg of phony app  (F applied to a lambda)
thf(ax2, axiom, ?[F: ($i > $o) > $o]: (F @ (^[X: $i]: (p @ X)))).
