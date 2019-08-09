/*-----------------------------------------------------------------------

File  : che_enigmaweightemb.c

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

#include "che_enigmaweightemb.h"


/*---------------------------------------------------------------------*/
/*                        Global Variables                             */
/*---------------------------------------------------------------------*/

/*---------------------------------------------------------------------*/
/*                      Forward Declarations                           */
/*---------------------------------------------------------------------*/

/*---------------------------------------------------------------------*/
/*                         Internal Functions                          */
/*---------------------------------------------------------------------*/

/*
static unsigned conj_indices[2048]; // TODO: dynamic alloc
static float conj_data[2048]; // TODO: dynamic alloc
*/

char*  EmbFileName = NULL;

static void symbols_count_increase(FunCode f_code, NumTree_p* counts)
{
   NumTree_p node = NumTreeFind(counts, f_code);
   if (node)
   {
      node->val1.i_val++;
   }
   else
{
      node = NumTreeCellAllocEmpty();
      node->key = f_code;
      node->val1.i_val = 1;
      NumTreeInsert(counts, node);
   }
}

static void symbols_count_term(Term_p term, NumTree_p* counts, int* vars)
{
   if (TermIsVar(term))
   {
      (*vars)++;
      return;
   }

   symbols_count_increase(term->f_code, counts);

   for (int i=0; i<term->arity; i++)
   {
      symbols_count_term(term->args[i], counts, vars);
   }
}

static void symbols_count_clause(Clause_p clause, NumTree_p* counts, int* vars)
{
   for (Eqn_p lit = clause->literals; lit; lit = lit->next)
   {
      symbols_count_term(lit->lterm, counts, vars);
      if (lit->rterm->f_code != SIG_TRUE_CODE)
      {
         symbols_count_increase(0, counts); // equality
         symbols_count_term(lit->rterm, counts, vars);
      }
   }
}

static void emb_add(double* out, double* emb, int mult)
{
   int i;
   for (i=0; i<EMB_LEN; i++)
   {
      out[i] += (mult * emb[i]);
   }
}

static void emb_div(double* out, int fact)
{
   int i;
   
   if (fact == 0)
   {
      return;
   }

   for (i=0; i<EMB_LEN; i++)
   {
      out[i] /= fact;
   }
}

static void emb_null(double* vec)
{
   int i;
   for (i=0; i<EMB_LEN; i++) 
   {
      vec[i] = 0.0;
   }
}

/*
static void emb_print_full(double* vec)
{
   fprintf(GlobalOut, "(");
   for (int i=0; i<EMB_LEN; i++)
   {
      fprintf(GlobalOut, "%f, ", vec[i]);
   }
   fprintf(GlobalOut, ")");
}
*/

static void emb_print(double* vec)
{
   fprintf(GlobalOut, "(%f,%f,%f,...,%f,%f,%f)",
         vec[0], vec[1], vec[2], vec[EMB_LEN-3], vec[EMB_LEN-2], vec[EMB_LEN-1]);
}

/*
 * stats[0] : the count of occurences of function symbols with arity == 0
 * stats[1] : the count of occurences of function symbols with arity == 1
 * stats[2] : the count of occurences of function symbols with arity == 2
 * stats[3] : the count of occurences of function symbols with arity == 3
 * stats[4] : the count of occurences of function symbols with arity == 4
 * stats[5] : the count of occurences of function symbols with arity >= 5
 * stats[6] : the count of occurences of predicate symbols with arity == 0
 * stats[7] : the count of occurences of predicate symbols with arity == 1
 * stats[8] : the count of occurences of predicate symbols with arity == 2 (without equality)
 * stats[9] : the count of occurences of predicate symbols with arity == 3
 * stats[10] : the count of occurences of predicate symbols with arity == 4
 * stats[11] : the count of occurences of predicate symbols with arity >= 5
 * stats[12] : the count of occurences of the equality predicate
 * stats[13 == STATS_COUNT_OFFSET] : the count of function symbols with arity == 0
 * stats[14] : the count of function symbols with arity == 1
 * stats[15] : the count of function symbols with arity == 2
 * stats[16] : the count of function symbols with arity == 3
 * stats[17] : the count of function symbols with arity == 4
 * stats[18] : the count of function symbols with arity >= 5
 * stats[19] : the count of predicate symbols with arity == 0
 * stats[20] : the count of predicate symbols with arity == 1
 * stats[21] : the count of predicate symbols with arity == 2 (without equality)
 * stats[22] : the count of predicate symbols with arity == 3
 * stats[23] : the count of predicate symbols with arity == 4
 * stats[24] : the count of predicate symbols with arity >= 5
 */
static void emb_clause_add(double* vec, int* stats, Clause_p clause, EnigmaWeightEmbParam_p data, int* mem, int* vars)
{
   NumTree_p counts = NULL;
   NumTree_p cnode;

   symbols_count_clause(clause, &counts, vars);

   PStack_p stack = NumTreeTraverseInit(counts);
   while ((cnode = NumTreeTraverseNext(stack)))
   {
      if (cnode->key == 0)
      {
         stats[12] += cnode->val1.i_val;
      }
      else
      {
         int offset = SigIsPredicate(data->ocb->sig, cnode->key) ? 6 : 0;
         int arity = MIN(SigFindArity(data->ocb->sig, cnode->key), 5);
         stats[arity+offset] += cnode->val1.i_val;
         stats[arity+offset+STATS_COUNT_OFFSET]++;
      }

      NumTree_p enode = NumTreeFind(&data->embeds, cnode->key);
      if (!enode) 
      {
         continue;
      }

      double* emb = enode->val1.p_val;
      emb_add(vec, emb, cnode->val1.i_val);
      (*mem) += cnode->val1.i_val;
   }
   NumTreeTraverseExit(stack);

   NumTreeFree(counts);
}

static void extweight_init(EnigmaWeightEmbParam_p data)
{
   static char str[256];
   double val;
   int ret;

   if (data->inited) 
   {
      return;
   }

   // read embeddings file
   if (!EmbFileName)
   {  
      Error("No embeddings file specified on the command line (--emb-file)!", USAGE_ERROR);
   }

   FILE* embs = fopen(EmbFileName, "r");
   if (!embs)
   {  
      Error("Can not open embeddings file '%s'!", FILE_ERROR, EmbFileName);
   }

   while (true)
   {
      ret = fscanf(embs, "%255[a-zA-Z0-9_=]", str);
      if (ret != 1)
      {
         Error("Wrong embeddings file format (last correct symbol: '%s')!", USAGE_ERROR, str);
      }
      //printf("%s (%d)\n", str, ret);

      double* vec = SizeMalloc(sizeof(double)*EMB_LEN); 
      for (int i=0; i<EMB_LEN; i++)
      {
         fscanf(embs, "%lf ", &val);
         vec[i] = val;
         //printf("%.3f ", val);
      }
      //printf("\n");
      
      bool is_eq = ((str[0]=='=') && (str[1]=='\0'));

      FunCode f_code = SigFindFCode(data->ocb->sig, str);
      if ((f_code == 0) && (!is_eq))
      {
         Warning("Unknown embedding symbol '%s'. Skipped.", str);
         SizeFree(vec, sizeof(double)*EMB_LEN);
      }
      else
      {
         // equality embedding is stored at f_code key 0
         NumTreeStore(&data->embeds, f_code, (IntOrP)(void*)vec, (IntOrP)NULL); 
      }

      if (feof(embs)) { break; }
   }
   fclose(embs);

   // debug print
   if (OutputLevel >= 1)
   {
      PStack_p stack = NumTreeTraverseInit(data->embeds);
      NumTree_p node;
      while ((node = NumTreeTraverseNext(stack)))
      {
         char* name = (node->key > 0) ? SigFindName(data->ocb->sig, node->key) : "=";
         fprintf(GlobalOut, "# emb(%s) = ", name);
         double* vec = node->val1.p_val;
         emb_print(vec);
         fprintf(GlobalOut, "\n");
      }
      NumTreeTraverseExit(stack);
   }

   // compute conjecture embedding
   int mem = 0;
   Clause_p clause;
   Clause_p anchor;

   emb_null(data->conj_emb);
   data->conj_len = 0;
   data->conj_vars = 0;
   anchor = data->proofstate->axioms->anchor;
   for (clause=anchor->succ; clause!=anchor; clause=clause->succ)
   {
      if (ClauseQueryTPTPType(clause) == CPTypeNegConjecture) 
      {
         emb_clause_add(data->conj_emb, data->conj_stats, clause, data, &mem, &data->conj_vars);
         data->conj_len += ClauseWeight(clause,1,1,1,1,1,false);
      }
   }
   emb_div(data->conj_emb, mem+1);
   data->conj_len += data->conj_stats[12]; // add equality symbols
   data->prob_spec = SpecFeatureCellAlloc();
   SpecFeaturesCompute(data->prob_spec, data->proofstate->axioms, data->ocb->sig);

   if (OutputLevel >= 1)
   {
      fprintf(GlobalOut, "# emb(GOAL) = "); 
      emb_print(data->conj_emb);
      fprintf(GlobalOut, "\n");
   }
   
   // load model
   XGBoosterCreate(NULL, 0, &data->xgboost_model);

   if (XGBoosterLoadModel(data->xgboost_model, data->model_filename) != 0)
   {
      Error("ENIGMA: Failed loading XGBoost model '%s':\n%s", FILE_ERROR,
         data->model_filename, XGBGetLastError());
   }
   //XGBoosterSetAttr(data->xgboost_model, "objective", "binary:logistic");

   data->inited = true;
 

   fprintf(GlobalOut, "# ENIGMA: XGBoost Embeddings model '%s' loaded.\n", 
      data->model_filename);
}

/*---------------------------------------------------------------------*/
/*                         Exported Functions                          */
/*---------------------------------------------------------------------*/

EnigmaWeightEmbParam_p EnigmaWeightEmbParamAlloc(void)
{
   EnigmaWeightEmbParam_p res = EnigmaWeightEmbParamCellAlloc();

   res->ocb = NULL;
   res->inited = false;
   for (int i=0; i<STATS_LEN; i++)
   {
      res->conj_stats[i] = 0;
   }

   return res;
}

void EnigmaWeightEmbParamFree(EnigmaWeightEmbParam_p junk)
{
   EnigmaWeightEmbParamCellFree(junk);
   SpecFeatureCellFree(junk->prob_spec);
   // TODO: free embeds, xgboost_model
}
 
WFCB_p EnigmaWeightEmbParse(
   Scanner_p in,  
   OCB_p ocb, 
   ProofState_p state)
{   
   ClausePrioFun prio_fun;
   //double len_mult;

   AcceptInpTok(in, OpenBracket);
   prio_fun = ParsePrioFun(in);
   AcceptInpTok(in, Comma);
   char* d_prefix = ParseFilename(in);
   //AcceptInpTok(in, Comma);
   //len_mult = ParseFloat(in);
   AcceptInpTok(in, CloseBracket);

   char* d_enigma = getenv("ENIGMA_ROOT");
   if (!d_enigma) {
      d_enigma = "Enigma";
   }

   DStr_p f_model = DStrAlloc();
   DStrAppendStr(f_model, d_enigma);
   DStrAppendStr(f_model, "/");
   DStrAppendStr(f_model, d_prefix);
   DStrAppendStr(f_model, "/");
   DStrAppendStr(f_model, "model.emb");
   char* model_filename = SecureStrdup(DStrView(f_model));
   DStrFree(f_model);

   return EnigmaWeightEmbInit(
      prio_fun, 
      ocb,
      state,
      model_filename);
}

WFCB_p EnigmaWeightEmbInit(
   ClausePrioFun prio_fun, 
   OCB_p ocb,
   ProofState_p proofstate,
   char* model_filename)
{
   EnigmaWeightEmbParam_p data = EnigmaWeightEmbParamAlloc();

   data->init_fun   = extweight_init;
   data->ocb        = ocb;
   data->proofstate = proofstate;
   data->inited     = false;
   data->embeds     = NULL;
   
   data->model_filename = model_filename;
   
   return WFCBAlloc(
      EnigmaWeightEmbCompute, 
      prio_fun,
      EnigmaWeightEmbExit, 
      data);
}

static void xgb_append(float val, unsigned* indices, float* data, int* cur)
{
   indices[*cur] = (*cur)+1;
   data[*cur] = val;
   (*cur)++;
}

double EnigmaWeightEmbCompute(void* data, Clause_p clause)
{
   static unsigned xgb_indices[2048]; // TODO
   static float xgb_data[2048]; // TODO
   int stats[STATS_LEN] = { 0 };
   EnigmaWeightEmbParam_p local;
   local = data;
   local->init_fun(data);

   static double emb[EMB_LEN];
   long long start = GetUSecClock();
      
   int clen = (int)ClauseWeight(clause,1,1,1,1,1,false);
   int cvars = 0;
   int mem = 0;
   emb_null(emb);
   emb_clause_add(emb, stats, clause, local, &mem, &cvars);
   emb_div(emb, mem+1);
   clen += stats[12]; // add equality symbols
   
   if (OutputLevel >= 1)
   {
      fprintf(GlobalOut, "# EVAL = "); 
      ClausePrint(GlobalOut, clause, true);
      fprintf(GlobalOut, "\n# emb(EVAL) = "); 
      emb_print(emb);
      fprintf(GlobalOut, "\n");
   }

   // compute xgb vector: clause part
   int i;
   int cur = 0;
   for (i=0; i<EMB_LEN; i++)
   {
      xgb_append(emb[i], xgb_indices, xgb_data, &cur);
   }
   xgb_append(clen, xgb_indices, xgb_data, &cur);
   xgb_append(cvars, xgb_indices, xgb_data, &cur);
   for (i=0; i<STATS_LEN; i++)
   {
      xgb_append(stats[i], xgb_indices, xgb_data, &cur);
   }
   // ... conjecture part
   for (i=0; i<EMB_LEN; i++)
   {
      xgb_append(local->conj_emb[i], xgb_indices, xgb_data, &cur);
   }
   xgb_append(local->conj_len, xgb_indices, xgb_data, &cur);
   xgb_append(local->conj_vars, xgb_indices, xgb_data, &cur);
   for (i=0; i<STATS_LEN; i++)
   {
      xgb_append(local->conj_stats[i], xgb_indices, xgb_data, &cur);
   }
   // ... problem features
   xgb_append(local->prob_spec->goals, xgb_indices, xgb_data, &cur);
   xgb_append(local->prob_spec->axioms, xgb_indices, xgb_data, &cur);
   xgb_append(local->prob_spec->clauses, xgb_indices, xgb_data, &cur);
   xgb_append(local->prob_spec->literals, xgb_indices, xgb_data, &cur);
   xgb_append(local->prob_spec->term_cells, xgb_indices, xgb_data, &cur);
   xgb_append(local->prob_spec->unitgoals, xgb_indices, xgb_data, &cur);
   xgb_append(local->prob_spec->unitaxioms, xgb_indices, xgb_data, &cur);
   xgb_append(local->prob_spec->horngoals, xgb_indices, xgb_data, &cur);
   xgb_append(local->prob_spec->hornaxioms, xgb_indices, xgb_data, &cur);
   xgb_append(local->prob_spec->eq_clauses, xgb_indices, xgb_data, &cur);
   xgb_append(local->prob_spec->peq_clauses, xgb_indices, xgb_data, &cur);
   xgb_append(local->prob_spec->groundunitaxioms, xgb_indices, xgb_data, &cur);
   xgb_append(local->prob_spec->groundgoals, xgb_indices, xgb_data, &cur);
   xgb_append(local->prob_spec->groundpositiveaxioms, xgb_indices, xgb_data, &cur);
   xgb_append(local->prob_spec->positiveaxioms, xgb_indices, xgb_data, &cur);
   xgb_append(local->prob_spec->ng_unit_axioms_part, xgb_indices, xgb_data, &cur);
   xgb_append(local->prob_spec->ground_positive_axioms_part, xgb_indices, xgb_data, &cur);
   xgb_append(local->prob_spec->max_fun_arity, xgb_indices, xgb_data, &cur);
   xgb_append(local->prob_spec->avg_fun_arity, xgb_indices, xgb_data, &cur);
   xgb_append(local->prob_spec->sum_fun_arity, xgb_indices, xgb_data, &cur);
   xgb_append(local->prob_spec->clause_max_depth, xgb_indices, xgb_data, &cur);
   xgb_append(local->prob_spec->clause_avg_depth, xgb_indices, xgb_data, &cur);

   int total = EMB_LEN+2+STATS_LEN+EMB_LEN+2+STATS_LEN+22;

   if (OutputLevel >= 2)
   {
      fprintf(GlobalOut, "# xgb vector: ");
      for (i=0; i<total; i++)
      {
         fprintf(GlobalOut, "%d:%f ", xgb_indices[i], xgb_data[i]);
      }
      fprintf(GlobalOut, "\n");
   }

   size_t xgb_nelem = total; 
   size_t xgb_num_col = 1 + total;
   size_t xgb_nindptr = 2;
   static bst_ulong xgb_indptr[2];
   xgb_indptr[0] = 0L;
   xgb_indptr[1] = xgb_nelem;
   DMatrixHandle xgb_matrix = NULL;
   if (XGDMatrixCreateFromCSREx(xgb_indptr, xgb_indices, xgb_data, 
          xgb_nindptr, xgb_nelem, xgb_num_col, &xgb_matrix) != 0)
   {
      Error("ENIGMA: Failed creating XGBoost prediction matrix:\n%s", 
         OTHER_ERROR, XGBGetLastError());
   }

   bst_ulong out_len = 0L;
   const float* pred;
   if (XGBoosterPredict(local->xgboost_model, xgb_matrix, 
          0, 0, &out_len, &pred) != 0)
   {
      Error("ENIGMA: Failed computing XGBoost prediction:\n%s", 
         OTHER_ERROR, XGBGetLastError());
   }
   
   //res = 1 + ((1.0 - pred[0]) * 10.0);
   double res;
   if (pred[0] < 0.5) { res = 10.0; } else { res = 1.0; }

   XGDMatrixFree(xgb_matrix);

   if (OutputLevel>=1) 
   {
      fprintf(GlobalOut, "=%.2f (val=%.3f,t=%.3fms,clen=%d,vlen=%ld) : ", res, pred[0], (double)(GetUSecClock() - start)/ 1000.0, clen, xgb_nelem);
      ClausePrint(GlobalOut, clause, true);
      fprintf(GlobalOut, "\n");
   }

   return res;
}

void EnigmaWeightEmbExit(void* data)
{
   EnigmaWeightEmbParam_p junk = data;
   
   EnigmaWeightEmbParamFree(junk);
}

/*---------------------------------------------------------------------*/
/*                        End of File                                  */
/*---------------------------------------------------------------------*/

