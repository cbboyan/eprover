%------------------------------------------------------------------------------
% Bug     : bug005-nonbool-toplevel-lambda-segfault
% Source  : Synthesized directly; found while fixing binder/equation scoping
%           in the TPTP parser (CLAUSES/ccl_tformulae.c)
% Triggers: SIGSEGV in TFormulaNNF (CLAUSES/ccl_tcnf.c) during clausification
% Formulae: 2 (1 type, 1 axiom)
%
% The axiom is ILL-TYPED: a thf annotated formula must have type $o, but
%   ^ [X: $o] : ( p & X )
% has type $o > $o. E does not reject it; instead the un-applied lambda
% reaches clausification, and TFormulaNNF descends through the '&' into the
% De Bruijn-bound variable in argument position 2 and crashes.
%
% Expected: a type error ("expected $o but got $o > $o"), not a segfault.
%
% Note the argument position matters: ( X & p ) and ( X = p ) do NOT crash,
% only ( p & X ) and ( p = X ) do.
%------------------------------------------------------------------------------
thf(p_decl,type,p: $o).
thf(c,axiom, ^ [X: $o] : ( p & X ) ).
%------------------------------------------------------------------------------
