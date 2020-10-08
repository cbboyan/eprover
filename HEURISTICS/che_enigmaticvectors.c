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

#define _GNU_SOURCE

#include "che_enigmaticvectors.h"
#include <stdlib.h>


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

static int hist_cmp(const long* x, const long* y, long* hist)
{
   return hist[(*y)-1] - hist[(*x)-1];
}

static void update_hist(long* hist, long count, NumTree_p node)
{
   if (count < 0) { return; }
   int i = node->val1.i_val - 1;
   if (i >= count)
   {
      i = count - 1;
   }
   hist[i]++;
}

static void update_rat(float* rat, long* hist, long count, int div)
{
   int i;
   for (i=0; i<count; i++)
   {
      rat[i] = div ? (float)hist[i] / div : 0;
   }
}

static void update_count(long* count, long* hist, long len)
{
   if (len < 0) { return; }
   int i;
   for (i=0; i<len; i++)
   {
      count[i] = i+1;
   }
   qsort_r(count, len, sizeof(long), 
      (int (*)(const void*, const void*, void*))hist_cmp, hist);
}

static void update_hists(EnigmaticClause_p enigma, EnigmaticInfo_p info)
{
   int vars = 0;
   int funcs = 0;
   int preds = 0;
   NumTree_p node;
   PStack_p stack = NumTreeTraverseInit(info->occs);
   while ((node = NumTreeTraverseNext(stack)))
   {
      if (node->key < 0) 
      {
         update_hist(enigma->var_hist, enigma->params->count_var, node);
         vars++;
      }
      else
      {
         if (SigIsPredicate(info->sig, node->key))
         {
            update_hist(enigma->pred_hist, enigma->params->count_sym, node);
            preds++;
         }
         else
         {
            update_hist(enigma->func_hist, enigma->params->count_sym, node);
            funcs++;
         }
      }

   }
   NumTreeTraverseExit(stack);

   update_count(enigma->var_count, enigma->var_hist, enigma->params->count_var);
   update_count(enigma->func_count, enigma->func_hist, enigma->params->count_sym);
   update_count(enigma->pred_count, enigma->pred_hist, enigma->params->count_sym);

   update_rat(enigma->var_rat, enigma->var_hist, enigma->params->count_var, vars);
   update_rat(enigma->func_rat, enigma->func_hist, enigma->params->count_sym, funcs);
   update_rat(enigma->pred_rat, enigma->pred_hist, enigma->params->count_sym, preds);
}

/*---------------------------------------------------------------------*/
/*                         Exported Functions                          */
/*---------------------------------------------------------------------*/

void EnigmaticClause(EnigmaticClause_p enigma, Clause_p clause, EnigmaticInfo_p info)
{
   EnigmaticInfoReset(info);

   info->var_offset = 0;
   update_clause(enigma, info, clause);
   enigma->avg_depth /= enigma->lits;
   update_hists(enigma, info);
}

void EnigmaticClauseSet(EnigmaticClause_p enigma, ClauseSet_p set, EnigmaticInfo_p info)
{
   EnigmaticInfoReset(info);
   
   info->var_offset = 0;
   Clause_p anchor = set->anchor;
   for (Clause_p clause=anchor->succ; clause!=anchor; clause=clause->succ)
   {
      update_clause(enigma, info, clause);
   }
   enigma->avg_depth /= enigma->lits;
   update_hists(enigma, info);
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

