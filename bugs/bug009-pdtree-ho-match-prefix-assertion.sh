# Isolated from bug006's investigation (see bug006-sup-partial-app-type-mismatch.md,
# "Root-cause investigation" section) -- found while re-running bug006's
# reproducer against a build with the eta-reduction fix applied. A separate,
# unrelated defect, in HO discrimination-tree matching, not sup/type-mismatch.
#
# Isolated from the ~1700-clause point in bug006's run where the crash first
# appeared: all 169 type declarations, all 111 clauses that were in the
# demodulator index at that point (dumped via gdb, `ClauseSetPrint`), plus
# the one clause being forward-contracted (`i_0_1703` in that run, renamed
# `target` here). No special flags needed -- plain -s reproduces it.
setarch $(uname -m) -R eprover-ho -s --tstp-format \
  bug009-pdtree-ho-match-prefix-assertion.p
