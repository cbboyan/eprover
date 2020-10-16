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

#define _GNU_SOURCE // for qsort_r

#include <stdlib.h>
#include "che_enigmaticvectors.h"
#include <che_clausesetfeatures.h>


/*---------------------------------------------------------------------*/
/*                        Global Variables                             */
/*---------------------------------------------------------------------*/

#define DEPTH(info) ((info)->path->current - ((info)->pos ? 1 : 0))
#define ARITY_IDX(f_code)  (MIN(SigFindArity(info->sig,(f_code)), enigma->params->count_arity-1))


/*---------------------------------------------------------------------*/
/*                      Forward Declarations                           */
/*---------------------------------------------------------------------*/

/*---------------------------------------------------------------------*/
/*                         Internal Functions                          */
/*---------------------------------------------------------------------*/

static __inline__ bool is_goal(FormulaProperties props)
{
   return ((props == CPTypeNegConjecture) ||
           (props == CPTypeConjecture) ||
           (props == CPTypeHypothesis));
}

static int hist_cmp(const long* x, const long* y, long* hist)
{
   return hist[(*y)-1] - hist[(*x)-1];
}

static __inline__ unsigned long hash_sdbm(unsigned long hash, int c)
{
   return (c + (hash << 6) + (hash << 16) - hash);
}

static unsigned long hash_update(unsigned long *hash, char* str, DStr_p out)
{
   if (out)
   {
      DStrAppendStr(out, str);
   }

   int c;
   while ((c = *str++))
   {
      *hash = hash_sdbm(*hash, c);
   }

   return *hash;
}

static __inline__ unsigned long hash_base(unsigned long* hash, long base)
{
   *hash = (*hash) % base;
   return *hash;
}

static bool symbol_skolem(char* name)
{
   if (name[0] == 'e') // be fast
   {
      if ((strncmp(name, "esk", 3) == 0) || (strncmp(name, "epred", 5) == 0))
      {
         return true;
      }
   }
   return false;
}

static bool symbol_internal(char* name)
{
   return (name[0] == '$');
}

static char* symbol_string(EnigmaticClause_p enigma, EnigmaticInfo_p info, FunCode f_code)
{
   static char str[128];
   static char postfix[16] = "\0";
      
   if (f_code < 0)               { return ENIGMATIC_VAR; }
   if (f_code == SIG_TRUE_CODE)  { return ENIGMATIC_POS; }
   if (f_code == SIG_FALSE_CODE) { return ENIGMATIC_NEG; }

   char* name = SigFindName(info->sig, f_code);
   bool skolem = symbol_skolem(name);
   if ((!(skolem || enigma->params->anonymous)) || (symbol_internal(name)))
   {
      return name;
   }

   char* sk = skolem ? ENIGMATIC_SKO : "";
   char prefix = SigIsPredicate(info->sig, f_code) ? 'p' : 'f'; 
   int arity = SigFindArity(info->sig, f_code);
   // TODO: if enigma->params->anonymous_sine
   //          && f_code < info->sine_symb_count:
   //          snprintf(postfix, 16, "^%ld", info->sine_symb_rank[f_code]);
   sprintf(str, "%s%c%d%s", sk, prefix, arity, postfix);
   
   StrTree_p node = StrTreeFind(&info->name_cache, str);
   if (!node)
   {
      node = StrTreeStore(&info->name_cache, str, (IntOrP)0L, (IntOrP)0L);
   }
   return node->key; // node->key is now a ("global") copy of str
}

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

static void update_feature_inc(NumTree_p* map, unsigned long fid)
{
   NumTree_p node = NumTreeFind(map, fid);
   if (!node) 
   {
      node = NumTreeCellAllocEmpty();
      node->key = fid;
      node->val1.i_val = 0;
      NumTreeInsert(map, node);
   }
   node->val1.i_val++;
}

static void update_feature_max(NumTree_p* map, unsigned long fid, long val)
{
   NumTree_p node = NumTreeFind(map, fid);
   if (!node) 
   {
      node = NumTreeCellAllocEmpty();
      node->key = fid;
      node->val1.i_val = 0;
      NumTreeInsert(map, node);
   }
   node->val1.i_val = MAX(val, node->val1.i_val);
}

static void update_stats(unsigned long fid, EnigmaticInfo_p info, DStr_p fstr)
{
   if (!fstr) { return; }
   StrTree_p node = StrTreeFind(&info->hashes, DStrView(fstr));
   if (!node)
   {
      node = StrTreeStore(&info->hashes, DStrView(fstr), (IntOrP)(long)fid, (IntOrP)0L);
   }
   node->val2.i_val++; // increase the usage ('encounter') counter
}

static void update_verts(EnigmaticClause_p enigma, EnigmaticInfo_p info, Term_p term)
{
   int i;

   if (enigma->params->offset_vert < 0) { return; }

   long len = enigma->params->length_vert;
   if ((!TermIsVar(term)) && 
       (!TermIsConst(term)) && 
       ((info->path->current < len) || (len == 0)))
   { 
      // not enough symbols yet && not yet the end && not infinite paths
      return; 
   }
   if (len == 0)
   {
      len = info->path->current; // infinite paths length
   }
   long begin = info->path->current - len;
   if (begin < 0)
   {
      len += begin;
      begin = 0;
   }
   
   unsigned long fid = 0; // feature id
   DStr_p fstr = info->collect_hashes ? DStrAlloc() : NULL;
   for (i=0; i<len; i++)
   {
      FunCode f_code = info->path->stack[begin+i].i_val;
      hash_update(&fid, symbol_string(enigma, info, f_code), fstr);
      hash_update(&fid, ":", fstr);
   }
   hash_base(&fid, enigma->params->base_vert);
   update_feature_inc(&enigma->vert, fid);
   update_stats(fid, info, fstr);
   if (fstr) { DStrFree(fstr); }
}

static void update_horiz(EnigmaticClause_p enigma, EnigmaticInfo_p info, Term_p term)
{
   if (enigma->params->offset_horiz < 0) { return; }

   unsigned long fid = 0;
   DStr_p fstr = info->collect_hashes ? DStrAlloc() : NULL;
   hash_update(&fid, symbol_string(enigma, info, term->f_code), fstr);
   hash_update(&fid, ".", fstr);
   for (int i=0; i<term->arity; i++)
   {
      hash_update(&fid, symbol_string(enigma, info, term->args[i]->f_code), fstr);
      hash_update(&fid, ".", fstr);
   }
   hash_base(&fid, enigma->params->base_horiz);
   update_feature_inc(&enigma->horiz, fid);
   update_stats(fid, info, fstr);
   if (fstr) { DStrFree(fstr); }
}

static void update_counts(EnigmaticClause_p enigma, EnigmaticInfo_p info, Term_p term)
{
   if (enigma->params->offset_count < 0) { return; }

   unsigned long fid = 0;
   DStr_p fstr = info->collect_hashes ? DStrAlloc() : NULL;
   char* sign = info->pos ? ENIGMATIC_POS : ENIGMATIC_NEG;
   hash_update(&fid, "#", fstr);
   hash_update(&fid, sign , fstr);
   hash_update(&fid, symbol_string(enigma, info, term->f_code), fstr);
   hash_base(&fid, enigma->params->base_count);
   update_feature_inc(&enigma->counts, fid);
   update_stats(fid, info, fstr);
   if (fstr) { DStrFree(fstr); }
}

static void update_depths(EnigmaticClause_p enigma, EnigmaticInfo_p info, Term_p term)
{
   if (enigma->params->offset_depth < 0) { return; }

   unsigned long fid = 0;
   DStr_p fstr = info->collect_hashes ? DStrAlloc() : NULL;
   char* sign = info->pos ? ENIGMATIC_POS : ENIGMATIC_NEG;
   hash_update(&fid, "%", fstr);
   hash_update(&fid, sign , fstr);
   hash_update(&fid, symbol_string(enigma, info, term->f_code), fstr);
   hash_base(&fid, enigma->params->base_depth);
   update_feature_max(&enigma->depths, fid, DEPTH(info));
   update_stats(fid, info, fstr);
   if (fstr) { DStrFree(fstr); }
}

static void update_paths(EnigmaticClause_p enigma, EnigmaticInfo_p info, Term_p term)
{
   update_verts(enigma, info, term);
   update_horiz(enigma, info, term);
   update_counts(enigma, info, term);
   update_depths(enigma, info, term);
}

static void update_prios(EnigmaticClause_p enigma, Clause_p clause)
{
   for (int i=0; i<EFC_PRIOS; i++)
   {
      enigma->prios[i] += ecb_prios[i](clause);
   }
}

static void update_arity(EnigmaticClause_p enigma, EnigmaticInfo_p info, long* arity, FunCode f_code)
{
   if (enigma->params->offset_arity < 0) { return; }
   arity[ARITY_IDX(f_code)] += 1;
}

static void update_arities(EnigmaticClause_p enigma, EnigmaticInfo_p info, Term_p term)
{
   if (TermIsVar(term)) 
   {
      enigma->vars_occs++;
   }
   else if (SigIsPredicate(info->sig, term->f_code))
   {
      update_arity(enigma, info, enigma->arity_pred_occs, term->f_code);
      enigma->preds_occs++;
   }
   else
   {
      update_arity(enigma, info, enigma->arity_func_occs, term->f_code);
      enigma->funcs_occs++;
   }
}

static void update_term(EnigmaticClause_p enigma, EnigmaticInfo_p info, Term_p term)
{
   PStackPushInt(info->path, term->f_code);
   if (TermIsVar(term) || TermIsConst(term))
   {
      enigma->width++;
      enigma->depth = MAX(enigma->depth, DEPTH(info));
   }
   else
   {
      for (int i=0; i<term->arity; i++)
      {
         update_term(enigma, info, term->args[i]);
      }
   }
   enigma->len++;
   update_arities(enigma, info, term);
   update_occurrences(enigma, info, term);
   update_paths(enigma, info, term);
   PStackPop(info->path);
}

static void update_lit(EnigmaticClause_p enigma, EnigmaticInfo_p info, Eqn_p lit)
{
   info->pos = EqnIsPositive(lit);
   PStackPushInt(info->path, info->pos ? SIG_TRUE_CODE : SIG_FALSE_CODE);
   if (lit->rterm->f_code == SIG_TRUE_CODE)
   {
      update_term(enigma, info, lit->lterm);
      if (info->pos) { enigma->pos_atoms++; } else { enigma->neg_atoms++; }
   }
   else
   {
      PStackPushInt(info->path, info->sig->eqn_code);
      enigma->len++; // count equality
      update_term(enigma, info, lit->lterm);
      update_term(enigma, info, lit->rterm);
      if (info->pos) { enigma->pos_eqs++; } else { enigma->neg_eqs++; }
   }
   if (info->pos) { enigma->pos++; } else { enigma->neg++; enigma->len++; }
   enigma->lits++;
   PStackReset(info->path);
}

static void update_clause(EnigmaticClause_p enigma, EnigmaticInfo_p info, Clause_p clause)
{
   info->var_distinct = 0;
   long max_depth = enigma->depth;
   for (Eqn_p lit=clause->literals; lit; lit=lit->next)
   {
      enigma->depth = 0;
      update_lit(enigma, info, lit);
      enigma->avg_lit_depth += enigma->depth; // temporarily the sum of literal depths
      max_depth = MAX(max_depth, enigma->depth);
   }
   enigma->depth = max_depth;
   info->var_offset += (2 * info->var_distinct);
   update_prios(enigma, clause);
   enigma->avg_lit_width = enigma->width / enigma->lits;
   enigma->avg_lit_len = enigma->len / enigma->lits;
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
         enigma->vars_count++;
         if (node->val1.i_val == 1) { enigma->vars_unique++; } else { enigma->vars_shared++; }
      }
      else
      {
         if (SigIsPredicate(info->sig, node->key))
         {
            update_hist(enigma->pred_hist, enigma->params->count_sym, node);
            update_arity(enigma, info, enigma->arity_pred_hist, node->key);
            preds++;
            enigma->preds_count++;
            if (node->val1.i_val == 1) { enigma->preds_unique++; } else { enigma->preds_shared++; }
         }
         else
         {
            update_hist(enigma->func_hist, enigma->params->count_sym, node);
            update_arity(enigma, info, enigma->arity_func_hist, node->key);
            funcs++;
            enigma->funcs_count++;
            if (node->val1.i_val == 1) { enigma->funcs_unique++; } else { enigma->funcs_shared++; }
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

Clause_p EnigmaticFormulaToClause(WFormula_p formula, EnigmaticInfo_p info)
{
   Eqn_p lits = EqnAlloc(formula->tformula, info->bank->false_term, info->bank, true);
   Clause_p encode = ClauseAlloc(lits);
   return encode;
}

void EnigmaticClause(EnigmaticClause_p enigma, Clause_p clause, EnigmaticInfo_p info)
{
   EnigmaticInfoReset(info);

   info->var_offset = 0;
   update_clause(enigma, info, clause);
   enigma->avg_lit_depth /= enigma->lits;
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
   enigma->avg_lit_depth /= enigma->lits;
   update_hists(enigma, info);
   if (enigma->params->use_prios) 
   {
      for (int i=0; i<EFC_PRIOS; i++)
      {
         enigma->prios[i] /= set->members; // average priorities
      }
   }
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

void EnigmaticProblem(EnigmaticVector_p vector, ClauseSet_p problem, EnigmaticInfo_p info)
{
   SpecFeature_p spec = SpecFeatureCellAlloc();
   SpecFeaturesCompute(spec, problem, info->sig);
   SpecLimits_p limits = CreateDefaultSpecLimits();
   SpecFeaturesAddEval(spec, limits);

   vector->problem_features[ 0] = spec->axiomtypes;
   vector->problem_features[ 1] = spec->goaltypes;
   vector->problem_features[ 2] = spec->eq_content;
   vector->problem_features[ 3] = spec->ng_unit_content;
   vector->problem_features[ 4] = spec->ground_positive_content;
   vector->problem_features[ 5] = spec->goals_are_ground;
   vector->problem_features[ 6] = spec->set_clause_size;
   vector->problem_features[ 7] = spec->set_literal_size;
   vector->problem_features[ 8] = spec->set_termcell_size;
   vector->problem_features[ 9] = spec->max_fun_ar_class;
   vector->problem_features[10] = spec->avg_fun_ar_class;
   vector->problem_features[11] = spec->sum_fun_ar_class;
   vector->problem_features[12] = spec->max_depth_class;
   vector->problem_features[13] = spec->clauses;
   vector->problem_features[14] = spec->goals;
   vector->problem_features[15] = spec->axioms;
   vector->problem_features[16] = spec->literals;
   vector->problem_features[17] = spec->term_cells;
   vector->problem_features[18] = spec->clause_max_depth;
   vector->problem_features[19] = spec->clause_avg_depth;
   vector->problem_features[20] = spec->unit;
   vector->problem_features[21] = spec->unitgoals;
   vector->problem_features[22] = spec->unitaxioms;
   vector->problem_features[23] = spec->horn;
   vector->problem_features[24] = spec->horngoals;
   vector->problem_features[25] = spec->hornaxioms;
   vector->problem_features[26] = spec->eq_clauses;
   vector->problem_features[27] = spec->peq_clauses;
   vector->problem_features[28] = spec->groundunitaxioms;
   vector->problem_features[29] = spec->positiveaxioms;
   vector->problem_features[30] = spec->groundpositiveaxioms;
   vector->problem_features[31] = spec->groundgoals;
   vector->problem_features[32] = spec->ng_unit_axioms_part;
   vector->problem_features[33] = spec->ground_positive_axioms_part;
   vector->problem_features[34] = spec->max_fun_arity;
   vector->problem_features[35] = spec->avg_fun_arity;
   vector->problem_features[36] = spec->sum_fun_arity;
   vector->problem_features[37] = spec->max_pred_arity;
   vector->problem_features[38] = spec->avg_pred_arity;
   vector->problem_features[39] = spec->sum_pred_arity;
   vector->problem_features[40] = spec->fun_const_count;
   vector->problem_features[41] = spec->fun_nonconst_count;
   vector->problem_features[42] = spec->pred_nonconst_count;             

   SpecFeatureCellFree(spec);
   SpecLimitsCellFree(limits);
}

void EnigmaticInitProblem(EnigmaticVector_p vector, EnigmaticInfo_p info, 
      FormulaSet_p f_axioms, ClauseSet_p axioms)
{
   Clause_p clause;
   bool free_clauses;
   ClauseSet_p theory = ClauseSetAlloc();
   ClauseSet_p goal = ClauseSetAlloc();
 
   // if FOF axioms are available, use them
   if (f_axioms->members)
   {
      WFormula_p handle;
      for (handle=f_axioms->anchor->succ; handle!=f_axioms->anchor; handle=handle->succ)
      {
         if (handle->is_clause) 
         {
            clause = WFormClauseToClause(handle);
         }
         else
         {
            clause = EnigmaticFormulaToClause(handle, info);
         }
         FormulaProperties props = FormulaQueryType(handle);
         ClauseSetInsert(is_goal(props) ? goal : theory, clause);
      }
      free_clauses = true;
   }
   // else use CNF axioms
   else
   {
      Clause_p anchor = axioms->anchor;
      for (clause=anchor->succ; clause!=anchor; clause=clause->succ)
      {
         FormulaProperties props = ClauseQueryTPTPType(clause);
         ClauseSetInsert(is_goal(props) ? goal : theory, clause);
      }
      free_clauses = false;
   }
   
   EnigmaticGoal(vector, goal, info);
   EnigmaticTheory(vector, theory, info);
   
   ClauseSet_p problem = ClauseSetAlloc();
   ClauseSetInsertSet(problem, theory); // this moves(!) clauses
   ClauseSetInsertSet(problem, goal);
   EnigmaticProblem(vector, problem, info);

   if (free_clauses) { ClauseSetFreeClauses(problem); }
   ClauseSetFree(theory);
   ClauseSetFree(goal);
   ClauseSetFree(problem);
}

void EnigmaticInitEval(char* features_filename, EnigmaticInfo_p* info, 
      EnigmaticVector_p* vector, ProofState_p proofstate)
{
   EnigmaticFeatures_p features = EnigmaticFeaturesLoad(features_filename);
   *vector = EnigmaticVectorAlloc(features);
   *info = EnigmaticInfoAlloc();
   (*info)->sig = proofstate->signature;
   (*info)->bank = proofstate->terms;
   (*info)->collect_hashes = false;
}

/*---------------------------------------------------------------------*/
/*                        End of File                                  */
/*---------------------------------------------------------------------*/

