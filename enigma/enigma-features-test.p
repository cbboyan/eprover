% Test file for ENIGMA HO feature encoding inspection.
% Covers: applied free vars, applied DB vars, lambdas, FO terms for comparison.
%
% Types: a, b, c are individuals; f/g are unary functions; h is binary.

thf(type_a, type, a : $i).
thf(type_b, type, b : $i).
thf(type_c, type, c : $i).
thf(type_f, type, f : $i > $i).
thf(type_g, type, g : $i > $i).
thf(type_h, type, h : $i > ($i > $i)).

% --- FO-style terms (baseline) ---

% f(a) and g(a): same arg, different function heads
thf(fo1, axiom, (f @ a) = (f @ a)).
thf(fo2, axiom, (g @ a) = (g @ a)).

% h(a,b) vs h(b,a): same head, swapped args
thf(fo3, axiom, (h @ a @ b) = (h @ a @ b)).
thf(fo4, axiom, (h @ b @ a) = (h @ b @ a)).

% --- Applied free vars (phony apps with var head) ---

% X @ a vs Y @ a: different heads, same arg
% Issue 1: vertical walk at 'a' should encode the head (X vs Y), but currently sees $@_var
thf(av1, axiom, ![X: $i > $i] : ((X @ a) = (X @ a))).
thf(av2, axiom, ![Y: $i > $i] : ((Y @ a) = (Y @ a))).

% X @ a @ b vs X @ b @ a: same head, swapped args
thf(av3, axiom, ![X: $i > $i > $i] : ((X @ a @ b) = (X @ a @ b))).
thf(av4, axiom, ![X: $i > $i > $i] : ((X @ b @ a) = (X @ b @ a))).

% X @ a @ b vs Y @ a @ b: different heads, same args
% Issue 1+2: both vertical and horizontal walks should distinguish X from Y
thf(av5, axiom, ![X: $i > $i > $i] : ((X @ a @ b) = (X @ a @ b))).
thf(av6, axiom, ![Y: $i > $i > $i] : ((Y @ a @ b) = (Y @ a @ b))).

% Nested: X @ (f @ a) — applied var with function-headed arg
thf(av7, axiom, ![X: $i > $i] : ((X @ (f @ a)) = (X @ (f @ a)))).

% Applied var under a function: f(X @ a)
thf(av8, axiom, ![X: $i > $i] : ((f @ (X @ a)) = (f @ (X @ a)))).

% --- Lambdas (DB lambda) ---

% ^[X]: f(X) vs ^[X]: g(X): same structure, different inner function
thf(lam1, axiom, (^[X: $i] : (f @ X)) = (^[X: $i] : (f @ X))).
thf(lam2, axiom, (^[X: $i] : (g @ X)) = (^[X: $i] : (g @ X))).

% ^[X,Y]: X vs ^[X,Y]: Y: Issue 3 — DB var index not encoded, look identical
thf(lam3, axiom, (^[X: $i, Y: $i] : X) = (^[X: $i, Y: $i] : X)).
thf(lam4, axiom, (^[X: $i, Y: $i] : Y) = (^[X: $i, Y: $i] : Y)).

% Lambda applied to arg (will be beta-reduced before clausification)
thf(lam5, axiom, ((^[X: $i] : (f @ X)) @ a) = (f @ a)).

% --- Conjecture (trivial, just to have one) ---

thf(goal, conjecture, a = a).
