%------------------------------------------------------------------------------
% File     : ALG247^2 : TPTP v9.2.1. Bugfixed v5.2.0.
% Domain   : Algebra
% Problem  : Push property lemma 0
% Version  : [Bro09] axioms : Reduced > Especial.
% English  :

% Refs     : [DHK95] Dowek et al. (1995), Higher-order Unification via Expl
%          : [Zha08] Zhang (2008), Using LEO-II to Prove Properties of an E
%          : [Ben09] Benzmueller (2009), Email to Geoff Sutcliffe
%          : [Bro09] Brown (2009), M-Set Models
% Source   : [Ben09]
% Names    : pushprop_lem0_lthm [Ben09]

% Status   : Theorem
% Rating   : 0.44 v9.1.0, 0.50 v8.2.0, 0.62 v8.1.0, 0.64 v7.5.0, 0.57 v7.4.0, 0.56 v7.2.0, 0.62 v7.0.0, 0.57 v6.4.0, 0.50 v6.3.0, 0.40 v6.2.0, 0.29 v5.5.0, 0.33 v5.4.0, 0.60 v5.2.0
% Syntax   : Number of formulae    :  238 ( 114 unt; 124 typ; 113 def)
%            Number of atoms       : 1218 ( 161 equ;   0 cnn)
%            Maximal formula atoms :    1 (  10 avg)
%            Number of connectives : 1942 (   6   ~;   0   |;   4   &; 916   @)
%                                         (   2 <=>;1014  =>;   0  <=;   0 <~>)
%            Maximal formula depth :    1 (   1 avg)
%            Number of types       :    3 (   2 usr)
%            Number of type conns  :  128 ( 128   >;   0   *;   0   +;   0  <<)
%            Number of symbols     :  123 ( 122 usr; 111 con; 0-4 aty)
%            Number of variables   :  327 (  39   ^; 283   !;   5   ?; 327   :)
% SPC      : TH0_THM_EQU_NAR

% Comments : 
% Bugfixes : v5.2.0 - Bugfixes in ALG003^0.ax
%------------------------------------------------------------------------------
%----Include Untyped Lambda Sigma defs
include('Axioms/ALG003^0.ax').
%------------------------------------------------------------------------------
thf(thm,conjecture,
    pushprop_lem0_lthm ).

%------------------------------------------------------------------------------
