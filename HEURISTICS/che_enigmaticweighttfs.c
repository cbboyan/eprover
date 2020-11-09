/*-----------------------------------------------------------------------

File  : che_enigmaticweighttf.c

Author: AI4REASON

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

#include "che_enigmaticweighttfs.h"
#include "cco_proofproc.h"

/*---------------------------------------------------------------------*/
/*                        Global Variables                             */
/*---------------------------------------------------------------------*/

/*---------------------------------------------------------------------*/
/*                      Forward Declarations                           */
/*---------------------------------------------------------------------*/

/*---------------------------------------------------------------------*/
/*                         Internal Functions                          */
/*---------------------------------------------------------------------*/


#ifdef DEBUG_ETF
static void debug_symbols(EnigmaticWeightTfsParam_p data)
{  
   PStack_p stack;
   NumTree_p node;
   
   fprintf(GlobalOut, "#TF# Symbols map:\n");
   fprintf(GlobalOut, "#TF# (conjecture):\n");
   stack = NumTreeTraverseInit(data->tensors->conj_syms);
   while ((node = NumTreeTraverseNext(stack)))
   {
      fprintf(GlobalOut, "#TF#   s%ld: %s\n", node->val1.i_val, node->key ? 
         SigFindName(data->proofstate->signature, node->key) : "=");
   }
   NumTreeTraverseExit(stack);
   
   fprintf(GlobalOut, "#TF# (clauses):\n");
   stack = NumTreeTraverseInit(data->tensors->syms);
   while ((node = NumTreeTraverseNext(stack)))
   {
      fprintf(GlobalOut, "#TF#   s%ld: %s\n", node->val1.i_val, node->key ? 
         SigFindName(data->proofstate->signature, node->key) : "=");
   }
   NumTreeTraverseExit(stack);
}

static void debug_terms(EnigmaticWeightTfsParam_p data)
{  
   PStack_p stack;
   NumTree_p node;
   
   fprintf(GlobalOut, "#TF# Terms map:\n");
   fprintf(GlobalOut, "#TF# (conjecture)\n");
   stack = NumTreeTraverseInit(data->tensors->conj_terms);
   while ((node = NumTreeTraverseNext(stack)))
   {
      fprintf(GlobalOut, "#TF#   t%ld: %s", node->val1.i_val, (node->key % 2 == 1) ? "~" : "");
      TermPrint(GlobalOut, node->val2.p_val, data->proofstate->signature, DEREF_ALWAYS);
      fprintf(GlobalOut, "\n");
   }
   NumTreeTraverseExit(stack);
   
   fprintf(GlobalOut, "#TF# (clauses)\n");
   stack = NumTreeTraverseInit(data->tensors->terms);
   while ((node = NumTreeTraverseNext(stack)))
   {
      fprintf(GlobalOut, "#TF#   t%ld: %s", node->val1.i_val, (node->key % 2 == 1) ? "~" : "");
      TermPrint(GlobalOut, node->val2.p_val, data->proofstate->signature, DEREF_ALWAYS);
      fprintf(GlobalOut, "\n");
   }
   NumTreeTraverseExit(stack);
}

static void debug_edges(EnigmaticWeightTfsParam_p data)
{
   long i;
   
   fprintf(GlobalOut, "#TF# Clause edges:\n");
   fprintf(GlobalOut, "#TF# (conjecture)\n");
   for (i=0; i<data->tensors->conj_cedges->current; i++)
   { 
      PDArray_p edge = PStackElementP(data->tensors->conj_cedges, i);
      fprintf(GlobalOut, "#TF#   (c%ld, t%ld)\n", 
         PDArrayElementInt(edge, 0), PDArrayElementInt(edge, 1));
   }
   fprintf(GlobalOut, "#TF# (clauses)\n");
   for (i=0; i<data->tensors->cedges->current; i++)
   { 
      PDArray_p edge = PStackElementP(data->tensors->cedges, i);
      fprintf(GlobalOut, "#TF#   (c%ld, t%ld)\n", 
         PDArrayElementInt(edge, 0), PDArrayElementInt(edge, 1));
   }

   fprintf(GlobalOut, "#TF# Term edges:\n");
   fprintf(GlobalOut, "#TF# (conjecture)\n");
   for (i=0; i<data->tensors->conj_tedges->current; i++)
   { 
      PDArray_p edge = PStackElementP(data->tensors->conj_tedges, i);
      fprintf(GlobalOut, "#TF#   (t%ld, t%ld, t%ld, s%ld, %ld)\n", 
         PDArrayElementInt(edge, 0), PDArrayElementInt(edge, 1),
         PDArrayElementInt(edge, 2), PDArrayElementInt(edge, 3),
         PDArrayElementInt(edge, 4));
   }
   fprintf(GlobalOut, "#TF# (clauses)\n");
   for (i=0; i<data->tensors->tedges->current; i++)
   { 
      PDArray_p edge = PStackElementP(data->tensors->tedges, i);
      fprintf(GlobalOut, "#TF#   (t%ld, t%ld, t%ld, s%ld, %ld)\n", 
         PDArrayElementInt(edge, 0), PDArrayElementInt(edge, 1),
         PDArrayElementInt(edge, 2), PDArrayElementInt(edge, 3),
         PDArrayElementInt(edge, 4));
   }

}
#endif

static void tfs_init(EnigmaticWeightTfsParam_p data)
{
   Clause_p clause;
   Clause_p anchor;

   if (data->inited)
   {
      return;
   }

   // process conjectures
   data->tensors->tmp_bank = TBAlloc(data->proofstate->signature);
   data->tensors->conj_mode = true;
   anchor = data->proofstate->axioms->anchor;
   for (clause=anchor->succ; clause!=anchor; clause=clause->succ)
   {
      if (ClauseQueryTPTPType(clause) == CPTypeNegConjecture) 
      {
         EnigmaticTensorsUpdateClause(clause, data->tensors);
      }
   }
   data->tensors->conj_mode = false;
   data->tensors->conj_maxvar = data->tensors->maxvar; // save maxvar to restore
   EnigmaticTensorsReset(data->tensors);

   data->sock->fd = socket(AF_INET , SOCK_STREAM , 0);
	if (data->sock->fd == -1)
	{
      perror("eprover: ENIGMATIC");
		Error("ENIGMATIC: Can not create socket to connect to TF server!", OTHER_ERROR);
	}

	data->sock->addr.sin_family = AF_INET;
   data->sock->addr.sin_addr.s_addr = inet_addr(data->server_ip);
	data->sock->addr.sin_port = htons(data->server_port);

   if (connect(data->sock->fd, (struct sockaddr*)&(data->sock->addr), 
       sizeof(data->sock->addr)) < 0)
   {
      perror("eprover: ENIGMATIC");
      Error("ENIGMATIC: Error connecting to the TF server '%s:%d'.", 
         OTHER_ERROR, data->server_ip, data->server_port);
   }

   fprintf(GlobalOut, "# ENIGMATIC: Connected to the TF server '%s:%d'.\n", 
      data->server_ip, data->server_port);
      
   if (data->lgb)
   {
      data->lgb->load_fun(data->lgb->model1);
      fprintf(GlobalOut, "# ENIGMATIC: LightGBM model '%s' loaded with %ld features '%s'\n",
         data->lgb->model1->model_filename, 
         data->lgb->model1->vector->features->count, 
         DStrView(data->lgb->model1->vector->features->spec)
      );
   }

   data->inited = true;
}

static float* tfs_eval_call(EnigmaticWeightTfsParam_p local)
{
#ifdef DEBUG_ETF
   debug_symbols(local);
   debug_terms(local);
   debug_edges(local);
#endif
   EnigmaticTensorsFill(local->tensors);
   EnigmaticSocketSend(local->sock, local->tensors);
   int n_q = local->tensors->fresh_c - local->tensors->conj_fresh_c + local->tensors->context_cnt;
   return EnigmaticSocketRecv(local->sock, n_q);
}

static void tfs_eval_gnn(EnigmaticWeightTfsParam_p local, ClauseSet_p set)
{
   Clause_p handle, handle0;
   long total = ClauseSetCardinality(set);
   int break_size = (total <= DelayedEvalSize*1.5) ? 0 : DelayedEvalSize;

   int done = 0; // how many done from set
   handle0 = set->anchor->succ; // beg of unevaled part in set
   while (done < total)
   {
      int size = 0;
      int skipped = 0;
      for (handle=handle0; handle!=set->anchor; handle=handle->succ)
      {
         if (handle->ext_weight == NAN) { skipped++; continue; } // skip evaluated
         EnigmaticTensorsUpdateClause(handle, local->tensors);
         size++;
         if (break_size && (size >= break_size)) { break; }
      }

      float* evals = tfs_eval_call(local);

      int idx = local->tensors->context_cnt;
      size = 0;
      for (handle=handle0; handle!=set->anchor; handle=handle->succ)
      {
         if (handle->ext_weight == NAN) { continue; } // skip evaluated
         handle->ext_weight = evals[idx++];
         size++;
         if (break_size && (size >= break_size)) { break; }
      }

      done += size + skipped;
      handle0 = handle;
      EnigmaticTensorsReset(local->tensors);
   }
}

static void tfs_eval_lgb(EnigmaticWeightTfsParam_p local, ClauseSet_p set)
{
   Clause_p handle;
   EnigmaticModel_p model = local->lgb->model1;
   for (handle=set->anchor->succ; handle!=set->anchor; handle=handle->succ)
   {
      double pred = EnigmaticPredictLgb(handle, local->lgb, model);
      double res = EnigmaticWeight(pred, model->weight_type, model->threshold);
      handle->ext_weight = (res == EW_POS) ? 0.0 : NAN;
   }
}

static void tfs_eval(ClauseSet_p set, void* data)
{
   EnigmaticWeightTfsParam_p local = data;
   local->init_fun(local);
   if (local->lgb)
   {
      tfs_eval_lgb(local, set);
   }
   tfs_eval_gnn(local, set);
}

static void tfs_processed(Clause_p clause, void* data)
{
   EnigmaticWeightTfsParam_p local = data;
   if (local->tensors->context_cnt >= local->context_size)
   {
      return;
   }
   local->init_fun(local);

   EnigmaticTensorsReset(local->tensors);
   local->tensors->conj_mode = true;
   EnigmaticTensorsUpdateClause(clause, local->tensors);
   local->tensors->conj_mode = false;
   local->tensors->conj_maxvar = local->tensors->maxvar; // save maxvar to restore
   EnigmaticTensorsReset(local->tensors);
#ifdef DEBUG_ETF
   fprintf(GlobalOut, "#TF# Context clause %ld added: ", local->tensors->context_cnt);
   ClausePrint(GlobalOut, clause, true);
   fprintf(GlobalOut, "\n");
#endif
   local->tensors->context_cnt++;
}


/*---------------------------------------------------------------------*/
/*                         Exported Functions                          */
/*---------------------------------------------------------------------*/

EnigmaticWeightTfsParam_p EnigmaticWeightTfsParamAlloc(void)
{
   EnigmaticWeightTfsParam_p res = EnigmaticWeightTfsParamCellAlloc();

   res->inited = false;
   res->tensors = EnigmaticTensorsAlloc();
   res->sock = EnigmaticSocketAlloc();
   res->lgb = NULL;

   return res;
}

void EnigmaticWeightTfsParamFree(EnigmaticWeightTfsParam_p junk)
{
   FREE(junk->server_ip);
   EnigmaticSocketFree(junk->sock);
   EnigmaticTensorsFree(junk->tensors);
   junk->tensors = NULL;
   if (junk->lgb)
   {
      EnigmaticWeightLgbParamFree(junk->lgb);
   }
   EnigmaticWeightTfsParamCellFree(junk);
}
 
WFCB_p EnigmaticWeightTfsParse(
   Scanner_p in,  
   OCB_p ocb, 
   ProofState_p state)
{   
   ClausePrioFun prio_fun;
   EnigmaticWeightLgbParam_p lgb = NULL;

   AcceptInpTok(in, OpenBracket);
   prio_fun = ParsePrioFun(in);
   AcceptInpTok(in, Comma);
   char* server_ip = ParseDottedId(in);
   AcceptInpTok(in, Comma);
   long server_port = ParseInt(in);
   AcceptInpTok(in, Comma);
   long context_size = ParseInt(in);
   AcceptInpTok(in, Comma);
   int weight_type = ParseInt(in);
   AcceptInpTok(in, Comma);
   double threshold = ParseFloat(in);
   if (TestInpTok(in, Comma))
   {
      NextToken(in);
      EnigmaticModel_p model = EnigmaticWeightParse(in, "model.lgb");
      lgb = EnigmaticWeightLgbParamAlloc();
      lgb->model1 = model;
      lgb->lgb_size = model->vector->features->count;
      lgb->lgb_indices = SizeMalloc(lgb->lgb_size*sizeof(int32_t));
      lgb->lgb_data = SizeMalloc(lgb->lgb_size*sizeof(float));
      if (model->weight_type != 1) 
      {
         Error("ENIGMATIC: In the two-phases evaluation, the LightGBM model must have binary weight type (1)!", USAGE_ERROR);
      }
   }
   AcceptInpTok(in, CloseBracket);

   return EnigmaticWeightTfsInit(
      prio_fun, 
      ocb,
      state,
      server_ip,
      server_port,
      context_size,
      weight_type,
      threshold,
      lgb);
}

WFCB_p EnigmaticWeightTfsInit(
   ClausePrioFun prio_fun, 
   OCB_p ocb,
   ProofState_p proofstate,
   char* server_ip,
   int server_port,
   long context_size,
   int weight_type,
   double threshold,
   EnigmaticWeightLgbParam_p lgb)
{
   EnigmaticWeightTfsParam_p data = EnigmaticWeightTfsParamAlloc();

   if (!DelayedEvalSize)
   {
      Error("ENIGMATIC: You must use --delayed-eval-cache with EnigmaticTfs weight function.", OTHER_ERROR);
   }

   data->init_fun   = tfs_init;
   data->ocb        = ocb;
   data->proofstate = proofstate;
   
   data->server_ip = server_ip;
   data->server_port = server_port;
   data->context_size = context_size;
   data->weight_type = weight_type;
   data->threshold = threshold;

   ProofStateDelayedEvalRegister(proofstate, tfs_eval, data);
   ProofStateClauseProcessedRegister(proofstate, tfs_processed, data);

   return WFCBAlloc(
      EnigmaticWeightTfsCompute, 
      prio_fun,
      EnigmaticWeightTfsExit, 
      data);
}

double EnigmaticWeightTfsCompute(void* data, Clause_p clause)
{  
   EnigmaticWeightTfsParam_p local = data;
   double weight;
   local->init_fun(data);

   if (clause->ext_weight == 0.0)
   {
      // default weight for unevaluated (initial) clauses
      weight = ClauseWeight(clause,1,1,1,1,1,1,true);
   }
   else if (clause->ext_weight == NAN)
   {
      weight = EW_WORST + ClauseWeight(clause,1,1,1,1,1,1,true);
   }
   else
   { 
      weight = EnigmaticWeight(clause->ext_weight, local->weight_type, local->threshold);
   }

//#if defined(DEBUG_ETF)
   fprintf(GlobalOut, "#TF#EVAL# %+.5f(%.1f)= ", weight, clause->ext_weight);
   ClausePrint(GlobalOut, clause, true);
   fprintf(GlobalOut, "\n");
//#endif

   return weight;
}

void EnigmaticWeightTfsExit(void* data)
{
   EnigmaticWeightTfsParam_p junk = data;
   if (junk->sock->fd > 0)
   {
      close(junk->sock->fd);
   }
   EnigmaticWeightTfsParamFree(junk);
}

/*---------------------------------------------------------------------*/
/*                        End of File                                  */
/*---------------------------------------------------------------------*/

