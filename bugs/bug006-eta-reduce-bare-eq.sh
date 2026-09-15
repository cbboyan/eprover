# Regression check for the suspected root cause of bug006: generic
# eta-reduction (TERMS/cte_lambda.c, reduce_eta_top_level/do_eta_reduce_db)
# has no guard for polymorphic symbols (SigIsPolymorphic, set for $eq/$neq
# in cte_signature.c), unlike every unification/matching call site that
# touches a polymorphic head. It happily strips ^[X,Y]:(X=Y) down to a bare,
# unapplied $eq -- exactly the shape TypeInferSort itself rejects at parse
# time ("Equality must have at least one argument", cte_typecheck.c:252-255)
# -- because stripping the last argument destroys the only thing pinning
# down $eq's type. See bug006-sup-partial-app-type-mismatch.md.
#
# Not a crash by itself -- this only checks that bare "$eq" (no following
# argument) shows up in -l2 output. Before the fix it does; after fixing
# reduce_eta_top_level/drop_args to stop short of arity 0 for polymorphic
# heads, it shouldn't.
cd "$(dirname "$0")"

eprover-ho -s -l2 --tstp-format bug006-eta-reduce-bare-eq.p 2>&1 | grep -n '\$eq\b'
