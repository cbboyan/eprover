%------------------------------------------------------------------------------
% File     : SYO548^1 : TPTP v9.2.1. Released v5.2.0.
% Domain   : Syntactic
% Problem  : hoice complement
% Version  : Especial.
% English  : There is an operator that chooses an element not in the predicate,
%            if there is one.

% Refs     : [Bac10] Backes (2010), Tableaux for Higher-Order Logic with If
%          : [Bro11] Brown E. (2011), Email to Geoff Sutcliffe
% Source   : [Bro11]
% Names    : CHOICE25 [Bro11]

% Status   : Theorem
% Rating   : 0.50 v8.2.0, 0.45 v8.1.0, 0.58 v7.4.0, 0.56 v7.3.0, 0.60 v7.2.0, 0.50 v7.1.0, 0.57 v7.0.0, 0.62 v6.4.0, 0.57 v6.3.0, 0.67 v6.2.0, 0.83 v6.1.0, 0.67 v6.0.0, 0.83 v5.5.0, 0.80 v5.4.0, 0.75 v5.2.0
% Syntax   : Number of formulae    :    1 (   0 unt;   0 typ;   0 def)
%            Number of atoms       :    0 (   0 equ;   0 cnn)
%            Maximal formula atoms :    0 (   0 avg)
%            Number of connectives :    6 (   2   ~;   0   |;   0   &;   3   @)
%                                         (   0 <=>;   1  =>;   0  <=;   0 <~>)
%            Maximal formula depth :    7 (   7 avg)
%            Number of types       :    2 (   0 usr)
%            Number of type conns  :    3 (   3   >;   0   *;   0   +;   0  <<)
%            Number of symbols     :    0 (   0 usr;   0 con; --- aty)
%            Number of variables   :    3 (   0   ^;   1   !;   2   ?;   3   :)
% SPC      : TH0_THM_NEQ_NAR

% Comments : 
%------------------------------------------------------------------------------
thf(choicecomp,conjecture,
    ? [E: ( $i > $o ) > $i] :
    ! [P: $i > $o] :
      ( ? [X: $i] :
          ~ ( P @ X )
     => ~ ( P @ ( E @ P ) ) ) ).

%------------------------------------------------------------------------------
