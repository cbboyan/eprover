/*-----------------------------------------------------------------------

File  : che_enigmaweighttf.c

Author: could be anyone

Contents
 
  Auto generated. Your comment goes here ;-).

  Copyright 2016 by the author.
  This code is released under the GNU General Public Licence.
  See the file COPYING in the main directory for details.
  Run "eprover -h" for contact information.

Changes

<1> Tue Mar  8 22:40:31 CET 2016
    New

-----------------------------------------------------------------------*/

#include "che_enigmaweighttf.h"


/*---------------------------------------------------------------------*/
/*                        Global Variables                             */
/*---------------------------------------------------------------------*/

/*---------------------------------------------------------------------*/
/*                      Forward Declarations                           */
/*---------------------------------------------------------------------*/

/*---------------------------------------------------------------------*/
/*                         Internal Functions                          */
/*---------------------------------------------------------------------*/

static int number_symbol(FunCode sym, EnigmaWeightTfParam_p data)
{
   NumTree_p node;

   node = NumTreeFind(&data->conj_syms, sym);
   if (!node)
   {
      if (!data->conj_mode) 
      {
         node = NumTreeFind(&data->syms, sym);
      }
      if (!node)
      {
         node = NumTreeCellAlloc();
         node->key = sym;
         node->val2.i_val = 0L;
         if (data->conj_mode)
         {
            node->val1.i_val = data->conj_fresh_s;
            data->conj_fresh_s++;
            NumTreeInsert(&data->conj_syms, node);
         }
         else
         {
            node->val1.i_val = data->fresh_s;
            data->fresh_s++;
            NumTreeInsert(&data->syms, node);
         }
      }
   }

   return node->val1.i_val;   
}

static int number_term(Term_p term, EnigmaWeightTfParam_p data)
{
   NumTree_p node;

   //long offset = data->conj_mode ? data->conj_vars_offset : data->vars_offset ;
   //long id = TermIsVar(term) ? term->f_code - offset : term->entry_no;
   long id = term->entry_no;

   node = NumTreeFind(&data->conj_terms, id);
   if (!node)
   {
      if (!data->conj_mode)
      {
         node = NumTreeFind(&data->terms, id);
      }
      if (!node)
      {
         node = NumTreeCellAlloc();
         node->key = id;
         node->val2.p_val = term;
         if (data->conj_mode)
         {
            node->val1.i_val = data->conj_fresh_t;
            data->conj_fresh_t++;
            NumTreeInsert(&data->conj_terms, node);
         }
         else
         {
            node->val1.i_val = data->fresh_t;
            data->fresh_t++;
            NumTreeInsert(&data->terms, node);
         }
      }
   }

   return node->val1.i_val;
}

static void names_update_term(Term_p term, EnigmaWeightTfParam_p data, bool pos)
{
   number_term(term, data);
   if (TermIsVar(term))
   {
      return;
   }

   number_symbol(term->f_code, data);
   for (int i=0; i<term->arity; i++)
   {
      names_update_term(term->args[i], data, pos);
   }
}

static Term_p fresh_term(Term_p term, EnigmaWeightTfParam_p data, DerefType deref)
{
   term = TermDeref(term, &deref);

   Term_p fresh;

   if (TermIsVar(term))
   {
      fresh = VarBankVarAssertAlloc(data->tmp_bank->vars, 
         term->f_code - data->maxvar, term->sort);      
   }
   else
   {
      fresh = TermTopCopyWithoutArgs(term);

      for(int i=0; i<term->arity; i++)
      {
         fresh->args[i] = fresh_term(term->args[i], data, deref);
      }
   }
   
   return fresh;
}

static void fresh_clause(Clause_p clause, EnigmaWeightTfParam_p data)
{
   for (Eqn_p lit = clause->literals; lit; lit = lit->next)
   {
      lit->lterm = fresh_term(lit->lterm, data, DEREF_ALWAYS);
      lit->rterm = fresh_term(lit->rterm, data, DEREF_ALWAYS);
      //lit->bank = NULL; // jai !!!
   }
}

static Clause_p clause_fresh_copy(Clause_p clause, EnigmaWeightTfParam_p data)
{
   Clause_p clause0 = ClauseFlatCopy(clause);
   fresh_clause(clause0, data);
   Clause_p clause1 = ClauseCopy(clause0, data->tmp_bank);
   ClauseFree(clause0);
   return clause1;
}

static void names_update_clause(Clause_p clause, EnigmaWeightTfParam_p data)
{
   Clause_p clause0 = clause_fresh_copy(clause, data); 

   for (Eqn_p lit = clause0->literals; lit; lit = lit->next)
   {
      bool pos = EqnIsPositive(lit);
      if (lit->rterm->f_code == SIG_TRUE_CODE)
      {
         names_update_term(lit->lterm, data, pos);
      }
      else
      {
         number_symbol(0, data); // 0 for equality "="
         names_update_term(lit->lterm, data, pos);
         names_update_term(lit->rterm, data, pos);
      }
   }
   data->maxvar = data->tmp_bank->vars->max_var;

   ClauseFree(clause0);
}

static void debug_symbols(EnigmaWeightTfParam_p data)
{  
   PStack_p stack;
   NumTree_p node;
   
   fprintf(GlobalOut, "#TF# Symbols map:\n");
   fprintf(GlobalOut, "#TF# (conjecture):\n");
   stack = NumTreeTraverseInit(data->conj_syms);
   while ((node = NumTreeTraverseNext(stack)))
   {
      fprintf(GlobalOut, "#TF#   s%ld: %s\n", node->val1.i_val, node->key ? 
         SigFindName(data->proofstate->signature, node->key) : "=");
   }
   NumTreeTraverseExit(stack);
   
   fprintf(GlobalOut, "#TF# (clauses):\n");
   stack = NumTreeTraverseInit(data->syms);
   while ((node = NumTreeTraverseNext(stack)))
   {
      fprintf(GlobalOut, "#TF#   s%ld: %s\n", node->val1.i_val, node->key ? 
         SigFindName(data->proofstate->signature, node->key) : "=");
   }
   NumTreeTraverseExit(stack);
}

static void debug_terms(EnigmaWeightTfParam_p data)
{  
   PStack_p stack;
   NumTree_p node;
   
   fprintf(GlobalOut, "#TF# Terms map:\n");
   fprintf(GlobalOut, "#TF# (conjecture)\n");
   stack = NumTreeTraverseInit(data->conj_terms);
   while ((node = NumTreeTraverseNext(stack)))
   {
      fprintf(GlobalOut, "#TF#   t%ld: ", node->val1.i_val);
      TermPrint(GlobalOut, node->val2.p_val, data->proofstate->signature, DEREF_ALWAYS);
      fprintf(GlobalOut, "\n");
   }
   NumTreeTraverseExit(stack);
   
   fprintf(GlobalOut, "#TF# (clauses)\n");
   stack = NumTreeTraverseInit(data->terms);
   while ((node = NumTreeTraverseNext(stack)))
   {
      fprintf(GlobalOut, "#TF#   t%ld: ", node->val1.i_val);
      TermPrint(GlobalOut, node->val2.p_val, data->proofstate->signature, DEREF_ALWAYS);
      fprintf(GlobalOut, "\n");
   }
   NumTreeTraverseExit(stack);
}

static void names_reset(EnigmaWeightTfParam_p data)
{
   if (data->terms)
   {
      NumTreeFree(data->terms);
      data->terms = NULL;
   }
   if (data->syms)
   {
      NumTreeFree(data->syms);
      data->syms = NULL;
   }

   data->fresh_t = data->conj_fresh_t;
   data->fresh_s = data->conj_fresh_s;
   data->fresh_c = data->conj_fresh_c;
   data->maxvar = data->conj_maxvar;
}

static void extweight_init(EnigmaWeightTfParam_p data)
{
   Clause_p clause;
   Clause_p anchor;

   if (data->inited)
   {
      return;
   }

   data->tmp_bank = TBAlloc(data->proofstate->signature);

   data->conj_mode = true;
   anchor = data->proofstate->axioms->anchor;
   for (clause=anchor->succ; clause!=anchor; clause=clause->succ)
   {
      if (ClauseQueryTPTPType(clause) == CPTypeNegConjecture) 
      {
         names_update_clause(clause, data);
      }
   }

   data->conj_mode = false;
   data->conj_maxvar = data->maxvar; // save maxvar to restore
   names_reset(data);
   data->inited = true;

   /*
   fprintf(GlobalOut, "# ENIGMA: TensorFlow(Mirecek) model '%s' loaded. (%s: %ld; conj_feats: %d; version: %ld)\n", 
      data->model_filename, 
      (data->enigmap->version & EFHashing) ? "hash_base" : "features", data->enigmap->feature_count, 
      data->conj_features_count, data->enigmap->version);
   */
}


/*---------------------------------------------------------------------*/
/*                         Exported Functions                          */
/*---------------------------------------------------------------------*/

EnigmaWeightTfParam_p EnigmaWeightTfParamAlloc(void)
{
   EnigmaWeightTfParam_p res = EnigmaWeightTfParamCellAlloc();

   res->inited = false;

   res->terms = NULL;
   res->syms = NULL;
   res->fresh_t = 1;
   res->fresh_s = 1;
   res->fresh_c = 1;
   
   res->conj_mode = false;
   res->conj_terms = NULL;
   res->conj_syms = NULL;
   res->conj_fresh_t = 1;
   res->conj_fresh_s = 1;
   res->conj_fresh_c = 1;

   res->maxvar = 0;

   return res;
}

void EnigmaWeightTfParamFree(EnigmaWeightTfParam_p junk)
{
   free(junk->model_dirname);

   EnigmaWeightTfParamCellFree(junk);
}
 
WFCB_p EnigmaWeightTfParse(
   Scanner_p in,  
   OCB_p ocb, 
   ProofState_p state)
{   
   ClausePrioFun prio_fun;
   double len_mult;

   AcceptInpTok(in, OpenBracket);
   prio_fun = ParsePrioFun(in);
   AcceptInpTok(in, Comma);
   char* d_model = ParseFilename(in);
   AcceptInpTok(in, Comma);
   len_mult = ParseFloat(in);
   AcceptInpTok(in, CloseBracket);

   return EnigmaWeightTfInit(
      prio_fun, 
      ocb,
      state,
      d_model,
      len_mult);
}

WFCB_p EnigmaWeightTfInit(
   ClausePrioFun prio_fun, 
   OCB_p ocb,
   ProofState_p proofstate,
   char* model_dirname,
   double len_mult)
{
   EnigmaWeightTfParam_p data = EnigmaWeightTfParamAlloc();

   data->init_fun   = extweight_init;
   data->ocb        = ocb;
   data->proofstate = proofstate;
   
   data->model_dirname = model_dirname;
   data->len_mult = len_mult;
   
   return WFCBAlloc(
      EnigmaWeightTfCompute, 
      prio_fun,
      EnigmaWeightTfExit, 
      data);
}

double EnigmaWeightTfCompute(void* data, Clause_p clause)
{
   EnigmaWeightTfParam_p local = data;
   local->init_fun(data);

   ClausePrint(GlobalOut, clause, true);
   fprintf(GlobalOut, "\n");

   names_update_clause(clause, local);
   debug_symbols(local);
   debug_terms(local);
   names_reset(local);

   return 1.0;
}

void EnigmaWeightTfExit(void* data)
{
   EnigmaWeightTfParam_p junk = data;
   
   EnigmaWeightTfParamFree(junk);
}

/*---------------------------------------------------------------------*/
/*                        End of File                                  */
/*---------------------------------------------------------------------*/

