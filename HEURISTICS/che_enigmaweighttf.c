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

static long number_symbol(FunCode sym, EnigmaWeightTfParam_p data)
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

static long number_term(Term_p term, long b, EnigmaWeightTfParam_p data)
{
   NumTree_p node;

   if (!term)
   {
      return -1;
   }
   // encode:
   // 1. variables (id<0)
   // 2. positive terms (even)
   // 3. negated terms (odd)
   long id = 2*term->entry_no; 
   if (b == -1)
   {
      id += 1;
   }

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

static Term_p fresh_term(Term_p term, EnigmaWeightTfParam_p data, DerefType deref)
{
   // NOTE: never call number_term from here as it updates maxvar
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

static void edge_clause(long cid, long tid, EnigmaWeightTfParam_p data)
{
   PDArray_p edge = PDArrayAlloc(2, 2);
   PDArrayAssignInt(edge, 0, cid);
   PDArrayAssignInt(edge, 1, tid);
   if (data->conj_mode)
   {
      PStackPushP(data->conj_cedges, edge);
   }
   else
   {
      PStackPushP(data->cedges, edge);
   }
}

static bool edge_term_check(long i, long j, long k, long l, long b,
   PStack_p edges)
{
   for (int idx=0; i<edges->current; i++)
   { 
      PDArray_p edge = PStackElementP(edges, idx);
      if (
         (PDArrayElementInt(edge, 0) == i) &&
         (PDArrayElementInt(edge, 1) == j) &&
         (PDArrayElementInt(edge, 2) == k) &&
         (PDArrayElementInt(edge, 3) == l) &&
         (PDArrayElementInt(edge, 4) == b))
      {
         return true;
      }
   }
   return false;
}

static void edge_term(long i, long j, long k, long l, long b,
      EnigmaWeightTfParam_p data)
{
   if (edge_term_check(i, j, k, l, b, data->conj_tedges) ||
       edge_term_check(i, j, k, l, b, data->tedges))
   {
      return;
   }

   PDArray_p edge = PDArrayAlloc(5, 5);
   PDArrayAssignInt(edge, 0, i);
   PDArrayAssignInt(edge, 1, j);
   PDArrayAssignInt(edge, 2, k);
   PDArrayAssignInt(edge, 3, l);
   PDArrayAssignInt(edge, 4, b);
   if (data->conj_mode)
   {
      PStackPushP(data->conj_tedges, edge);
   }
   else
   {
      PStackPushP(data->tedges, edge);
   }
}

static long names_update_term(Term_p term, EnigmaWeightTfParam_p data, long b)
{
   long tid = number_term(term, b, data);
   if (TermIsVar(term))
   {
      data->maxvar = MAX(data->maxvar, -term->f_code);
      return tid;
   }

   long sid = number_symbol(term->f_code, data);
   long tid0 = 0;
   long tid1 = 0;
   for (int i=0; i<term->arity; i++)
   {
      tid0 = tid1;
      tid1 = names_update_term(term->args[i], data, 1);
      if ((tid0 != 0) && (tid1 != 0))
      {
         edge_term(tid, tid0, tid1, sid, b, data);
      }
   }
   if (term->arity == 0)
   {
      edge_term(tid, -1, -1, sid, b, data);
   }
   if (term->arity == 1)
   {
      edge_term(tid, tid1, -1, sid, b, data);
   }

   return tid;
}

static void names_update_clause(Clause_p clause, EnigmaWeightTfParam_p data)
{
   Clause_p clause0 = clause_fresh_copy(clause, data); 

   long tid = -1;
   long cid = (data->conj_mode) ? data->conj_fresh_c : data->fresh_c;
   for (Eqn_p lit = clause0->literals; lit; lit = lit->next)
   {
      bool pos = EqnIsPositive(lit);
      if (lit->rterm->f_code == SIG_TRUE_CODE)
      {
         tid = names_update_term(lit->lterm, data, pos ? 1 : -1);
      }
      else
      {
         Term_p term = TermTopAlloc(data->proofstate->signature->eqn_code, 2);
         term->args[0] = lit->lterm;
         term->args[1] = lit->rterm;
         Term_p term1 = TBInsert(data->tmp_bank, term, DEREF_ALWAYS);
         tid = names_update_term(term1, data, pos ? 1 : -1);
         TermTopFree(term); 
      }
      edge_clause(cid, tid, data);
   }
   if (tid == -1)
   {
      return;
   }
   if (data->conj_mode)
   {
      data->conj_fresh_c++;
   }
   else
   {
      data->fresh_c++;
   }

   //DEBUG:
   fprintf(GlobalOut, "#TF# Clause c%ld: ", cid);
   ClausePrint(GlobalOut, clause, true);
   fprintf(GlobalOut, "\n");
   //

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
      fprintf(GlobalOut, "#TF#   t%ld: %s", node->val1.i_val, (node->key % 2 == 1) ? "~" : "");
      TermPrint(GlobalOut, node->val2.p_val, data->proofstate->signature, DEREF_ALWAYS);
      fprintf(GlobalOut, "\n");
   }
   NumTreeTraverseExit(stack);
   
   fprintf(GlobalOut, "#TF# (clauses)\n");
   stack = NumTreeTraverseInit(data->terms);
   while ((node = NumTreeTraverseNext(stack)))
   {
      fprintf(GlobalOut, "#TF#   t%ld: %s", node->val1.i_val, (node->key % 2 == 1) ? "~" : "");
      TermPrint(GlobalOut, node->val2.p_val, data->proofstate->signature, DEREF_ALWAYS);
      fprintf(GlobalOut, "\n");
   }
   NumTreeTraverseExit(stack);
}

static void debug_edges(EnigmaWeightTfParam_p data)
{
   long i;
   
   fprintf(GlobalOut, "#TF# Clause edges:\n");
   fprintf(GlobalOut, "#TF# (conjecture)\n");
   for (i=0; i<data->conj_cedges->current; i++)
   { 
      PDArray_p edge = PStackElementP(data->conj_cedges, i);
      fprintf(GlobalOut, "#TF#   (c%ld, t%ld)\n", 
         PDArrayElementInt(edge, 0), PDArrayElementInt(edge, 1));
   }
   fprintf(GlobalOut, "#TF# (clauses)\n");
   for (i=0; i<data->cedges->current; i++)
   { 
      PDArray_p edge = PStackElementP(data->cedges, i);
      fprintf(GlobalOut, "#TF#   (c%ld, t%ld)\n", 
         PDArrayElementInt(edge, 0), PDArrayElementInt(edge, 1));
   }

   fprintf(GlobalOut, "#TF# Term edges:\n");
   fprintf(GlobalOut, "#TF# (conjecture)\n");
   for (i=0; i<data->conj_tedges->current; i++)
   { 
      PDArray_p edge = PStackElementP(data->conj_tedges, i);
      fprintf(GlobalOut, "#TF#   (t%ld, t%ld, t%ld, s%ld, %ld)\n", 
         PDArrayElementInt(edge, 0), PDArrayElementInt(edge, 1),
         PDArrayElementInt(edge, 2), PDArrayElementInt(edge, 3),
         PDArrayElementInt(edge, 4));
   }
   fprintf(GlobalOut, "#TF# (clauses)\n");
   for (i=0; i<data->tedges->current; i++)
   { 
      PDArray_p edge = PStackElementP(data->tedges, i);
      fprintf(GlobalOut, "#TF#   (t%ld, t%ld, t%ld, s%ld, %ld)\n", 
         PDArrayElementInt(edge, 0), PDArrayElementInt(edge, 1),
         PDArrayElementInt(edge, 2), PDArrayElementInt(edge, 3),
         PDArrayElementInt(edge, 4));
   }

}

static void debug_vector(char* name, float* vals, int len)
{
   fprintf(GlobalOut, "%s = [ ", name);
   for (int i=0; i<len; i++)
   {
      fprintf(GlobalOut, "%d:%.0f%s", i, vals[i], (i<len-1) ? ", " : " ]\n");
   }
}

static void free_edges(PStack_p stack)
{
   while (!PStackEmpty(stack))
   {  
      PDArray_p edge = PStackPopP(stack);
      PDArrayFree(edge);
   }
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

   free_edges(data->cedges);
   free_edges(data->tedges);

   data->fresh_t = data->conj_fresh_t;
   data->fresh_s = data->conj_fresh_s;
   data->fresh_c = data->conj_fresh_c;
   data->maxvar = data->conj_maxvar;
}

static void tensor_fill_ini_nodes(float* vals, NumTree_p syms, 
   EnigmaWeightTfParam_p data)
{
   NumTree_p node;
   PStack_p stack;

   stack = NumTreeTraverseInit(syms);
   while ((node = NumTreeTraverseNext(stack)))
   {
      if (node->key < 0) 
      {
         vals[node->val1.i_val] = 2; // variable
      }
      else 
      {
         Term_p term = node->val2.p_val;
         if (SigIsPredicate(data->proofstate->signature, term->f_code))
         {
            vals[node->val1.i_val] = 1; // literal
         }
         else
         {
            vals[node->val1.i_val] = 0; // otherwise
         }
      }
   }
   NumTreeTraverseExit(stack);
}

static void tensor_fill_ini_symbols(float* vals, NumTree_p terms, 
   EnigmaWeightTfParam_p data)
{
   NumTree_p node;
   PStack_p stack;

   stack = NumTreeTraverseInit(terms);
   while ((node = NumTreeTraverseNext(stack)))
   {
      if (SigIsPredicate(data->proofstate->signature,  node->key)) 
      {
         vals[node->val1.i_val] = 1; // predicate
      }
      else 
      {
         vals[node->val1.i_val] = 0; // function
      }
   }
   NumTreeTraverseExit(stack);
}

static void tensor_fill_ini_clauses(float* vals, EnigmaWeightTfParam_p data)
{
   for (int i=0; i<data->fresh_c; i++)
   {
      vals[i] = (i < data->conj_fresh_c) ? 0 : 1;
   }
}

static void tensor_fill(EnigmaWeightTfParam_p data)
{
   static float ini_nodes[2048];
   static float ini_symbols[2048];
   static float ini_clauses[2048];

   tensor_fill_ini_nodes(ini_nodes, data->conj_terms, data);
   tensor_fill_ini_nodes(ini_nodes, data->terms, data);
   debug_vector("ini_nodes", ini_nodes, data->fresh_t);

   tensor_fill_ini_symbols(ini_symbols, data->conj_syms, data);
   tensor_fill_ini_symbols(ini_symbols, data->syms, data);
   debug_vector("ini_symbols", ini_symbols, data->fresh_s);

   tensor_fill_ini_clauses(ini_clauses, data);
   debug_vector("ini_clauses", ini_clauses, data->fresh_c);
   
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
   res->fresh_t = 0;
   res->fresh_s = 0;
   res->fresh_c = 0;
   res->tedges = PStackAlloc();
   res->cedges = PStackAlloc();
   
   res->conj_mode = false;
   res->conj_terms = NULL;
   res->conj_syms = NULL;
   res->conj_fresh_t = 0;
   res->conj_fresh_s = 0;
   res->conj_fresh_c = 0;
   res->conj_tedges = PStackAlloc();
   res->conj_cedges = PStackAlloc();

   res->ini_nodes = PStackAlloc(); 
   res->ini_symbols = PStackAlloc(); 
   res->ini_clauses = PStackAlloc(); 

   res->maxvar = 0;
   res->tmp_bank = NULL;

   return res;
}

void EnigmaWeightTfParamFree(EnigmaWeightTfParam_p junk)
{
   free(junk->model_dirname);

   free_edges(junk->tedges);
   free_edges(junk->cedges);
   free_edges(junk->conj_tedges);
   free_edges(junk->conj_cedges);
   PStackFree(junk->tedges);
   PStackFree(junk->cedges);
   PStackFree(junk->conj_tedges);
   PStackFree(junk->conj_cedges);

   PStackFree(junk->ini_nodes);
   PStackFree(junk->ini_symbols);
   PStackFree(junk->ini_clauses);

   if (junk->tmp_bank)
   {
      TBFree(junk->tmp_bank);
      junk->tmp_bank = NULL;
   }

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
   debug_edges(local);
   tensor_fill(local);

   names_reset(data);

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

