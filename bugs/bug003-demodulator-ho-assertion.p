%------------------------------------------------------------------------------
% Bug     : bug003-demodulator-ho-assertion
% Source  : Synthesized directly from bug description
% Triggers: eprover-ho-nf ccl_rewrite.c:632 indexed_find_demodulator Assertion `false'
% Formulae: 4 (2 type, 2 axiom)
%
% ax1 (demodulator): sum distributes over a binary op
%   sum(X |-> op(F(X), G(X)), A) = op(sum(F, A), sum(G, A))
%
% ax2 (Fubini/swap): sum(X |-> sum(G(X), A), A) = sum(Y |-> sum(X |-> G(X,Y), A), A)
%
% When E applies the ax1 demodulator to the subterm sum(^[Y]. sum(G(Y), A), A)
% from ax2, the HO unifier binds F or G to a lambda, creating a beta redex in the
% demodulator LHS that TermStructPrefixEqual (DEREF_ONCE) fails to reduce.
%------------------------------------------------------------------------------

thf(sy_sum,type, sum: ( a > a ) > set_a > a).
thf(sy_op,type, op: a > a > a).

thf(ax1,axiom,
    ! [F: a > a,G: a > a,A: set_a] :
      ( ( sum @ ^ [X: a] : ( op @ ( F @ X ) @ ( G @ X ) ) @ A )
      = ( op @ ( sum @ F @ A ) @ ( sum @ G @ A ) ) ) ).

thf(ax2,axiom,
    ! [A: set_a,G: a > a > a] :
      ( ( sum @ ^ [X: a] : ( sum @ ( G @ X ) @ A ) @ A )
      = ( sum @ ^ [Y: a] : ( sum @ ^ [X: a] : ( G @ X @ Y ) @ A ) @ A ) ) ).

%------------------------------------------------------------------------------
