/*-----------------------------------------------------------------------

File  : che_enigmaticvectors.c

Author: Stephan Schultz, AI4REASON

Contents
 
  Copyright 2020 by the authors.
  This code is released under the GNU General Public Licence.
  See the file COPYING in the main CLIB directory for details.
  Run "eprover -h" for contact information.

Changes

<1> Fri 10 Apr 2020 11:14:30 PM CEST

-----------------------------------------------------------------------*/


#include "che_enigmaticvectors.h"


/*---------------------------------------------------------------------*/
/*                        Global Variables                             */
/*---------------------------------------------------------------------*/

/*---------------------------------------------------------------------*/
/*                      Forward Declarations                           */
/*---------------------------------------------------------------------*/

/*---------------------------------------------------------------------*/
/*                         Internal Functions                          */
/*---------------------------------------------------------------------*/

static void update_occurrences(EnigmaticClause_p enigma, EnigmaticInfo_p info, Term_p term)
{
   FunCode f_code = term->f_code;
   if (TermIsVar(term))
   {
      f_code -= info->var_offset; // clause variable offset
   }
   else if (f_code <= info->sig->internal_symbols)
   {
      return; // ignore internal symbols
   }

   NumTree_p vnode = NumTreeFind(&(info->occs), f_code);
   if (vnode)
   {
      vnode->val1.i_val += 1;
   }
   else
   {
      vnode = NumTreeCellAllocEmpty();
      vnode->key = f_code;
      vnode->val1.i_val = 1;
      NumTreeInsert(&(info->occs), vnode);
      if (TermIsVar(term))
      {
         info->var_distinct++;
      }
   }
}

static void update_term(EnigmaticClause_p enigma, EnigmaticInfo_p info, Term_p term, long depth)
{
   if (TermIsVar(term) || TermIsConst(term))
   {
      enigma->width++;
      enigma->depth = MAX(enigma->depth, depth+1);
   }
   else
   {
      for (int i=0; i<term->arity; i++)
      {
         update_term(enigma, info, term->args[i], depth+1);
      }
   }
   enigma->len++;
   update_occurrences(enigma, info, term);
}

static void update_lit(EnigmaticClause_p enigma, EnigmaticInfo_p info, Eqn_p lit)
{
   bool pos = EqnIsPositive(lit);
   if (lit->rterm->f_code == SIG_TRUE_CODE)
   {
      update_term(enigma, info, lit->lterm, pos ? 0 : 1);
      if (pos) { enigma->pos_atoms++; } else { enigma->neg_atoms++; }
   }
   else
   {
      enigma->len++; // count equality
      info->sig = lit->bank->sig;
      update_term(enigma, info, lit->lterm, 1);
      update_term(enigma, info, lit->rterm, 1);
      if (pos) { enigma->pos_eqs++; } else { enigma->neg_eqs++; }
   }
   if (pos) { enigma->pos++; } else { enigma->neg++; enigma->len++; }
   enigma->lits++;
}

static void update_clause(EnigmaticClause_p enigma, EnigmaticInfo_p info, Clause_p clause)
{
   info->var_distinct = 0;
   long max_depth = enigma->depth;
   for (Eqn_p lit=clause->literals; lit; lit=lit->next)
   {
      enigma->depth = 0;
      update_lit(enigma, info, lit);
      enigma->avg_depth += enigma->depth; // temporarily the sum of literal depths
      max_depth = MAX(max_depth, enigma->depth);
   }
   enigma->depth = max_depth;
   info->var_offset += (2 * info->var_distinct);
}

/*---------------------------------------------------------------------*/
/*                         Exported Functions                          */
/*---------------------------------------------------------------------*/

void EnigmaticClause(EnigmaticClause_p enigma, Clause_p clause, EnigmaticInfo_p info)
{
   bool free_info;
   if (info)
   {
      EnigmaticInfoReset(info);
      free_info = false;
   }
   else
   {
      info = EnigmaticInfoAlloc();
      free_info = true;
   }

   info->var_offset = 0;
   update_clause(enigma, info, clause);
   enigma->avg_depth /= enigma->lits;
   
   if (free_info) { EnigmaticInfoFree(info); }
}

void EnigmaticClauseSet(EnigmaticClause_p enigma, ClauseSet_p set, EnigmaticInfo_p info)
{
   bool free_info;
   if (info)
   {
      EnigmaticInfoReset(info);
      free_info = false;
   }
   else
   {
      info = EnigmaticInfoAlloc();
      free_info = true;
   }
   
   info->var_offset = 0;
   Clause_p anchor = set->anchor;
   for (Clause_p clause=anchor->succ; clause!=anchor; clause=clause->succ)
   {
      update_clause(enigma, info, clause);
   }
   enigma->avg_depth /= enigma->lits;
   
   if (free_info) { EnigmaticInfoFree(info); }
}

void EnigmaticTheory(EnigmaticVector_p vector, ClauseSet_p axioms, EnigmaticInfo_p info)
{
   if (vector->theory)
   {
      EnigmaticClauseSet(vector->theory, axioms, info);
   }
}

void EnigmaticGoal(EnigmaticVector_p vector, ClauseSet_p goal, EnigmaticInfo_p info)
{
   if (vector->goal)
   {
      EnigmaticClauseSet(vector->goal, goal, info);
   }
}

/*---------------------------------------------------------------------*/
/*                        End of File                                  */
/*---------------------------------------------------------------------*/

