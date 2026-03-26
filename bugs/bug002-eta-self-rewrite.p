thf(ty_complex, type, complex: $tType).
thf(ty_comp,    type, comp_c130555887omplex: ( complex > complex ) > ( complex > complex ) > complex > complex ).
thf(ty_id,      type, id_complex: complex > complex).
thf(ty_deriv,   type, deriv_complex: ( complex > complex ) > complex > complex).
thf(ty_one,     type, one_one_complex: complex).
thf(ty_w,       type, w: complex).

thf(comp_apply, axiom,
    ( comp_c130555887omplex
    = ( ^ [F2: complex > complex, G: complex > complex, X: complex] : ( F2 @ ( G @ X ) ) ) ) ).
thf(comp_id,    axiom,
    ! [F: complex > complex] :
      ( ( comp_c130555887omplex @ F @ id_complex )
      = F ) ).
thf(id_apply,   axiom,
    ( id_complex
    = ( ^ [X: complex] : X ) ) ).
thf(deriv_id,   axiom,
    ( ( deriv_complex @ id_complex )
    = ( ^ [Z2: complex] : one_one_complex ) ) ).
thf(conj_0,     conjecture,
    ( ( deriv_complex @ id_complex @ w )
    = one_one_complex ) ).
