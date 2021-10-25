/*-----------------------------------------------------------------------

File  : cte_ho_bindings.c

Author: Petar Vukmirovic.

Contents

  Implementation of the module which creates higher-order variable
  bindings.

  Copyright 1998, 1999 by the author.
  This code is released under the GNU General Public Licence and
  the GNU Lesser General Public License.
  See the file COPYING in the main E directory for details..
  Run "eprover -h" for contact information.

Changes

<1> ma 25 okt 2021 10:35:21 CEST
    New

-----------------------------------------------------------------------*/

#include "cte_ho_bindings.h"
#include "cte_pattern_match_mgu.h"

#define CONSTRAINT_STATE(c) ((c) & 3)
#define CONSTRAINT_COUNTER(c) ((c) >> 2) // c must be unisigned!!!
// #define CONSTRAINT_COUNTER_ADD(c,x) ( ((CONSTRAINT_COUNTER(c) + x) << 2) | (CONSTRAINT_STATE(c)) )
// #define CONSTRAINT_COUNTER_INC(c) ( CONSTRAINT_COUNTER_ADD(c, 1))
#define BUILD_CONSTR(c, s) (((c)<<2)|s)

#define IMIT_MASK (63U)
#define PROJ_MASK (IMIT_MASK << 6)
#define IDENT_MASK (PROJ_MASK << 6)
#define ELIM_MASK (IDENT_MASK << 6)

#define GET_IMIT(c) ( (c) & IMIT_MASK )
#define GET_PROJ(c) ( ((c) & PROJ_MASK) >> 6 )
#define GET_IDENT(c) ( ((c) & IDENT_MASK) >> 12 )
#define GET_ELIM(c) ( ((c) & ELIM_MASK) >> 18 )


#define INC_IMIT(c) ( (GET_IMIT(c)+1) | (~IMIT_MASK & c) )
#define INC_PROJ(c) ( ((GET_PROJ(c)+1) << 6) | (~PROJ_MASK & c) )
#define INC_IDENT(c) ( ((GET_IDENT(c)+1) << 12) | (~IDENT_MASK & c) )
#define INC_ELIM(c) ( ((GET_ELIM(c)+1) << 18) | (~ELIM_MASK & c) )


/*---------------------------------------------------------------------*/
/*                        Global Variables                             */
/*---------------------------------------------------------------------*/

/*---------------------------------------------------------------------*/
/*                      Forward Declarations                           */
/*---------------------------------------------------------------------*/

/*---------------------------------------------------------------------*/
/*                         Internal Functions                          */
/*---------------------------------------------------------------------*/

/*-----------------------------------------------------------------------
//
// Function: build_imitation()
//
//   Builds imitation binding if rhs has a constant as the head.
//   Otherwise returns NULL.
//
// Global Variables: -
//
// Side Effects    : -
//
/----------------------------------------------------------------------*/

Term_p build_imitation(TB_p bank, Term_p flex, Term_p rhs)
{
   return NULL;
}

/*-----------------------------------------------------------------------
//
// Function: build_projection()
//
//   Projects onto argument idx if return type of variable at the head
//   of flex returns the same type as the argument. Otherwise returns NULL.
//
// Global Variables: -
//
// Side Effects    : -
//
/----------------------------------------------------------------------*/

Term_p build_projection(TB_p bank, Term_p flex, Term_p rhs, int idx)
{
   return NULL;
}

/*-----------------------------------------------------------------------
//
// Function: build_elim()
//
//   Eliminates argument idx. Always succeeds.
//
// Global Variables: -
//
// Side Effects    : -
//
/----------------------------------------------------------------------*/

Term_p build_elim(TB_p bank, Term_p flex, int idx)
{
   return NULL;
}

/*-----------------------------------------------------------------------
//
// Function: build_ident()
//
//   Builds identification binding. Must be called with both lhs and 
//   rhs top-level free variables. Then it returns.
//
// Global Variables: -
//
// Side Effects    : -
//
/----------------------------------------------------------------------*/

Term_p build_ident(TB_p bank, Term_p lhs, Term_p rhs)
{
   return NULL;
}

/*-----------------------------------------------------------------------
//
// Function: build_trivial_ident()
//
//   Builds trivial identification binding. Must be called with both lhs and 
//   rhs top-level free variables. Then it returns.
//
// Global Variables: -
//
// Side Effects    : -
//
/----------------------------------------------------------------------*/

Term_p build_trivial_ident(TB_p bank, Term_p lhs, Term_p rhs)
{
   return NULL;
}


/*---------------------------------------------------------------------*/
/*                         Exported Functions                          */
/*---------------------------------------------------------------------*/


/*-----------------------------------------------------------------------
//
// Function: SubstComputeFixpointMgu()
//
//   Assuming that flex is an (applied) variable and rhs an arbitrary term
//   which are normalized and to which substitution is applied generate
//   the next binding in an attempt to solve the problem flex =?= rhs. 
//   What the next binding is is determined by the value of 'state'.
//   The last two bits of 'state' have special meaning (is the variable
//   pair already processed) and the remaining bits determine how far
//   in the enumeration of bindings we are. 'applied_bs' counts how
//   many bindings of a certain kind are applied. It is a value that
//   is inspected through bit masks that give value of particular bindings.
//   Sets succ to true if substitution was changed
//
// Global Variables: -
//
// Side Effects    : -
//
/----------------------------------------------------------------------*/

ConstraintTag_t ComputeNextBinding(Term_p flex, Term_p rhs, 
                                   ConstraintTag_t state, Limits_t* applied_bs,
                                   TB_p bank, Subst_p subst,
                                   HeuristicParms_p parms, bool* succ)
{
   assert(TermIsTopLevelFreeVar(flex));
   ConstraintTag_t cnt = CONSTRAINT_COUNTER(state);
   ConstraintTag_t is_solved = CONSTRAINT_STATE(state);
   ConstraintTag_t res = 0;
   PStackPointer orig_subst = PStackGetSP(subst);

   if(is_solved != DECOMPOSED_VAR)
   {
      const int num_args_l = MAX(flex->arity-1, 0);
      const int num_args_r = TermIsTopLevelFreeVar(rhs) ? MAX(rhs->arity-1, 0) : 0;
      const int limit = 1 + 2*num_args_l + 2*num_args_r + 1;
                  // 1 for imitation
                  // 2*arguments for projection and eliminations
                  // 1 for identification

      while(res == 0 && cnt < limit)
      {
         if(cnt == 0)
         {
            cnt++;
            if(!TermIsAppliedFreeVar(rhs) &&
               GET_IMIT(*applied_bs) < parms->imit_limit)
            {
               Term_p target = build_imitation(bank, flex, rhs);
               if(target)
               {
                  // imitation building can fail if head is DB var
                  res = BUILD_CONSTR(cnt, state);
                  SubstAddBinding(subst, flex, target);
                  *applied_bs = INC_IMIT(*applied_bs);
               }
            }
         }
         else if((num_args_l != 0 || num_args_r != 0) 
                  && cnt <= num_args_l + num_args_r)
         {
            // sometimes we need to apply projection on both left
            // and right side
            bool left_side = num_args_l != 0 && cnt <= num_args_l;
            Term_p arg = 
               left_side ? flex->args[cnt] : rhs->args[cnt-num_args_l]; 
            Term_p hd_var = GetFVarHead(left_side ? flex : rhs);
            if(GetRetType(hd_var->type) == GetRetType(arg->type) &&
               (GET_PROJ(*applied_bs) < parms->func_proj_limit
               || !TypeIsArrow(arg->type)))
            {
               int offset = left_side ? 1 : num_args_l + 1;
               if(!left_side)
               {
                  SWAP(flex, rhs);
               }
               Term_p target = build_projection(bank, flex, rhs, cnt-offset);
               if(target)
               {
                  // building projection can fail if it is determined
                  // that it should not be generated
                  res = BUILD_CONSTR(cnt+1, state);
                  SubstAddBinding(subst, flex, target);
                  *applied_bs = INC_PROJ(*applied_bs);
               }
            }
            cnt++;
         }
         else if((num_args_l != 0 || num_args_r != 0) && 
                 cnt <= 2*(num_args_l+num_args_r))
         {
            // elimination -- currently computing only linear
            // applied variable so we do not subtract 1
            cnt++;
            if(GET_ELIM(*applied_bs) < parms->elim_limit)
            {
               bool left_side = num_args_l != 0 && cnt <= 2*num_args_l + num_args_r;
               if(!left_side)
               {
                  flex = rhs;
               }
               int offset = cnt - (left_side ? 1 : 2)*num_args_l - num_args_r;
               Term_p target = 
                  build_elim(bank, flex, cnt-offset);
               res = BUILD_CONSTR(cnt, state);
               SubstAddBinding(subst, GetFVarHead(flex), target);
               *applied_bs = INC_ELIM(*applied_bs);
            }
            else 
            {
               // skipping other arguments
               cnt = 2*(num_args_l+num_args_r)+1;
            }
         }
         else if(cnt == 2*(num_args_l+num_args_r)+1 && TermIsTopLevelFreeVar(rhs))
         {
            // identification
            cnt++;
            Term_p target = 
               (GET_IDENT(*applied_bs) < parms->ident_limit ? build_ident : build_trivial_ident)
               (bank, flex, rhs);
            res = BUILD_CONSTR(cnt, DECOMPOSED_VAR);
            *applied_bs = INC_IDENT(*applied_bs);
            SubstAddBinding(subst, flex, target);
         }
      }
   }
   *succ = PStackGetSP(subst) != orig_subst;
   return res;
}
