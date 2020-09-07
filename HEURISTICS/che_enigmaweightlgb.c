/*-----------------------------------------------------------------------

File  : che_enigmaweightlgb.c

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

#include "che_enigmaweightlgb.h"
#include <cco_sine.h>


/*---------------------------------------------------------------------*/
/*                        Global Variables                             */
/*---------------------------------------------------------------------*/

/*---------------------------------------------------------------------*/
/*                      Forward Declarations                           */
/*---------------------------------------------------------------------*/

/*---------------------------------------------------------------------*/
/*                         Internal Functions                          */
/*---------------------------------------------------------------------*/

#define MAX_FEATURES 20480

static int32_t conj_indices[MAX_FEATURES]; // TODO: dynamic alloc
static float conj_data[MAX_FEATURES]; // TODO: dynamic alloc

static void extweight_init(EnigmaWeightLgbParam_p data)
{
   if (data->lgb_model) 
   {
      return;
   }
  
   int iters;
   if (LGBM_BoosterCreateFromModelfile(data->model_filename, &iters, &data->lgb_model) != 0)
   {
      Error("ENIGMA: Failed loading LightGBM model '%s':\n%s", FILE_ERROR,
         data->model_filename, LGBM_GetLastError());
   }

   data->enigmap = EnigmapLoad(data->features_filename, data->ocb->sig);
   
   // problem features:
   if (data->enigmap->version & EFProblem)
   {
      EnigmapFillProblemFeatures(data->enigmap, data->proofstate->axioms);
   }

   //if (data->enigmap->version & EFSine)
   //{
   //   data->enigmap->symb_rank = SinESymbolRanking(data->proofstate->axioms, data->proofstate->terms);
   //   data->enigmap->symb_count = data->enigmap->sig->f_count+1;
   //}

   int len = 0;
   if (data->enigmap->version & EFConjecture)
   {
      Clause_p clause;
      Clause_p anchor;
      NumTree_p features = NULL;
      NumTree_p varstat = NULL;
      int varoffset = 0;

      anchor = data->proofstate->axioms->anchor;
      for (clause=anchor->succ; clause!=anchor; clause=clause->succ)
      {
         if(ClauseQueryTPTPType(clause)==CPTypeNegConjecture) 
         {
            len += FeaturesClauseExtend(&features, clause, data->enigmap);
            FeaturesAddClauseStatic(&features, clause, data->enigmap, &len, &varstat, &varoffset);
            //EnigmapMarkConjectureSymbols(data->enigmap, clause);
         }
      }
      FeaturesAddVariables(&features, &varstat, data->enigmap, &len);

      if (len >= MAX_FEATURES) { Error("ENIGMA: Too many conjecture features!", OTHER_ERROR); } 
  
      //printf("CONJ FEATURES: ");
      int i = 0;
      while (features) 
      {
         NumTree_p cell = NumTreeExtractEntry(&features,NumTreeMinNode(features)->key);
         conj_indices[i] = cell->key + data->enigmap->feature_count;
         conj_data[i] = (float)cell->val1.i_val;
         //printf("%d:%d ", conj_indices[i], (int)conj_data[i]);
         i++;
         NumTreeCellFree(cell);
      }
      //printf("\n");

      assert(i==len);
   }

   data->conj_features_count = len;
   data->conj_features_indices = conj_indices;
   data->conj_features_data = conj_data;

   fprintf(GlobalOut, "# ENIGMA: LightGBM model '%s' loaded. (%s: %ld; conj_feats: %d; version: %ld; iters: %d)\n", 
      data->model_filename, 
      (data->enigmap->version & EFHashing) ? "hash_base" : "features", data->enigmap->feature_count, 
      data->conj_features_count, data->enigmap->version, iters);
}

/*---------------------------------------------------------------------*/
/*                         Exported Functions                          */
/*---------------------------------------------------------------------*/

EnigmaWeightLgbParam_p EnigmaWeightLgbParamAlloc(void)
{
   EnigmaWeightLgbParam_p res = EnigmaWeightLgbParamCellAlloc();

   res->lgb_model = NULL;
   res->enigmap = NULL;

   return res;
}

void EnigmaWeightLgbParamFree(EnigmaWeightLgbParam_p junk)
{
   free(junk->model_filename);
   free(junk->features_filename);
   junk->lgb_model = NULL; // TODO: free model & enigmap

   EnigmaWeightLgbParamCellFree(junk);
}
 
WFCB_p EnigmaWeightLgbParse(
   Scanner_p in,  
   OCB_p ocb, 
   ProofState_p state)
{   
   ClausePrioFun prio_fun;
   double len_mult;

   AcceptInpTok(in, OpenBracket);
   prio_fun = ParsePrioFun(in);
   AcceptInpTok(in, Comma);
   char* d_prefix = ParseFilename(in);
   AcceptInpTok(in, Comma);
   len_mult = ParseFloat(in);
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
   DStrAppendStr(f_model, "model.lgb");
   char* model_filename = SecureStrdup(DStrView(f_model));
   DStrFree(f_model);

   DStr_p f_featmap = DStrAlloc();
   DStrAppendStr(f_featmap, d_enigma);
   DStrAppendStr(f_featmap, "/");
   DStrAppendStr(f_featmap, d_prefix);
   DStrAppendStr(f_featmap, "/");
   DStrAppendStr(f_featmap, "enigma.map");
   char* features_filename = SecureStrdup(DStrView(f_featmap));
   DStrFree(f_featmap);
  
   //fprintf(GlobalOut, "ENIGMA: MODEL: %s\n", model_filename);
   //fprintf(GlobalOut, "ENIGMA: FEATURES: %s\n", features_filename);

   free(d_prefix);

   return EnigmaWeightLgbInit(
      prio_fun, 
      ocb,
      state,
      model_filename,
      features_filename,
      len_mult);
}

WFCB_p EnigmaWeightLgbInit(
   ClausePrioFun prio_fun, 
   OCB_p ocb,
   ProofState_p proofstate,
   char* model_filename,
   char* features_filename,
   double len_mult)
{
   EnigmaWeightLgbParam_p data = EnigmaWeightLgbParamAlloc();

   data->init_fun   = extweight_init;
   data->ocb        = ocb;
   data->proofstate = proofstate;
   
   data->model_filename = model_filename;
   data->features_filename = features_filename;
   data->len_mult = len_mult;
   
   return WFCBAlloc(
      EnigmaWeightLgbCompute, 
      prio_fun,
      EnigmaWeightLgbExit, 
      data);
}

double EnigmaWeightLgbCompute(void* data, Clause_p clause)
{
   static int32_t lgb_indices[MAX_FEATURES]; // TODO: dynamic alloc
   static float lgb_data[MAX_FEATURES]; // TODO: dynamic alloc
   EnigmaWeightLgbParam_p local;
   double res = 1.0;
   
   local = data;
   local->init_fun(data);
   
   long long start = GetUSecClock();
   int len = 0;
   NumTree_p features = FeaturesClauseCollect(clause, local->enigmap, &len);
   //printf("features count: %d\n", len);
      
   if (len+local->conj_features_count+22 >= MAX_FEATURES) { Error("ENIGMA: Too many clause features!", OTHER_ERROR); }

   int i = 0;
   while (features) 
   {
      NumTree_p cell = NumTreeExtractEntry(&features,NumTreeMinNode(features)->key);
      lgb_indices[i] = cell->key;
      lgb_data[i] = (float)cell->val1.i_val;
      //printf("%d:%.0f ", lgb_indices[i], lgb_data[i]);
      i++;
      NumTreeCellFree(cell);
   }
   //printf("|");

   for (int j=0; j<local->conj_features_count; j++) 
   {
      lgb_indices[i+j] = local->conj_features_indices[j];
      lgb_data[i+j] = local->conj_features_data[j];
      //printf("%d:%.0f ", lgb_indices[i+j], lgb_data[i+j]);
   }

   //printf("|");
   int total = i+local->conj_features_count;
   if (local->enigmap->version & EFProblem)
   {
      for (int j=0; j<22; j++)
      {
         lgb_indices[total+j] = (2*local->enigmap->feature_count)+1+j; // TODO: fix when no conjecture features
         lgb_data   [total+j] = local->enigmap->problem_features[j];
         //printf("%d:%.3f ", lgb_indices[i+j+local->conj_features_count], lgb_data[i+j+local->conj_features_count]);
      }
      total += 22;
   }

   // TODO: fix proof watch & problem features
   
   // detect proofwatch version
   if (local->enigmap->version & EFProofWatch)
   {
      //printf("|");
      NumTree_p proof;
      PStack_p stack;
      int k = i + local->conj_features_count;
      int offset = local->enigmap->feature_count;
      if (local->enigmap->version & EFConjecture) 
      {
         offset += local->enigmap->feature_count;
      }

      stack = NumTreeTraverseInit(local->proofstate->wlcontrol->watch_progress);
      while((proof = NumTreeTraverseNext(stack)))
      {
         if (proof->val1.i_val == 0) 
         { 
            continue; 
         }
         NumTree_p len = NumTreeFind(&(local->proofstate->wlcontrol->proof_len), proof->key);
         if (!len)
         {
            Error("Watchlist: Unknown proof length of proof #%ld.  Should not happen!", 
               OTHER_ERROR, proof->key);
         }
         lgb_indices[k] = proof->key + offset;
         lgb_data[k] = ((double)proof->val1.i_val)/len->val1.i_val;
         //printf("%d:%.3f ", lgb_indices[k], lgb_data[k]);
         k++;
         if (k >= MAX_FEATURES) { Error("ENIGMA: Too many proof watch features!", OTHER_ERROR); }
      }
      NumTreeTraverseExit(stack);
      total = k;
   }
   //printf("\n");
   //printf("[duration] feature extract: %f.2 ms\n", (double)(GetUSecClock() - start)/1000.0);
   
   //start = clock();
   int64_t lgb_nelem = total;
   int64_t lgb_num_col = 1 + local->enigmap->feature_count +
      (local->enigmap->version & EFConjecture ? local->enigmap->feature_count : 0) +
      (local->enigmap->version & EFProblem ? 22 : 0) +
      (local->enigmap->version & EFProofWatch ? local->proofstate->wlcontrol->proofs_count : 0);
   int64_t lgb_nindptr = 2;
   static int32_t lgb_indptr[2];
   lgb_indptr[0] = 0;
   lgb_indptr[1] = lgb_nelem;
   
   //start = clock();
   int64_t out_len = 0L;
   double pred[2];
   if (LGBM_BoosterPredictForCSRSingleRow(local->lgb_model, lgb_indptr, C_API_DTYPE_INT32,
      lgb_indices, lgb_data, C_API_DTYPE_FLOAT32, lgb_nindptr, lgb_nelem,
      lgb_num_col, C_API_PREDICT_NORMAL, 0, "", &out_len, pred) != 0)
   {
      Error("ENIGMA: Failed computing LightGBM prediction:\n%s", 
         OTHER_ERROR, LGBM_GetLastError());
   }
   //printf("prediction: len=%ld first=%f\n", out_len, pred[0]);
   
   //res = 1 + ((1.0 - pred[0]) * 10.0);
   if (pred[0] <= 0.5) { res = 10.0; } else { res = 1.0; }

   double clen = ClauseWeight(clause,1,1,1,1,1,false);
   res = (clen * local->len_mult) + res;

   if (OutputLevel>=1) {
      fprintf(GlobalOut, "=%.2f (val=%.3f,t=%.3fms,clen=%.1f,vlen=%ld) : ", res, pred[0], (double)(GetUSecClock() - start)/ 1000.0, clen, lgb_nelem);
      ClausePrint(GlobalOut, clause, true);
      fprintf(GlobalOut, "\n");
   }
   
   //printf("[duration] lgb predict: %.3f ms   (clen=%.1f, vlen=%ld)\n", (double)(GetUSecClock() - start)/ 1000.0, clen, lgb_nelem);

   return res;
}

void EnigmaWeightLgbExit(void* data)
{
   EnigmaWeightLgbParam_p junk = data;
   
   EnigmaWeightLgbParamFree(junk);
}

/*---------------------------------------------------------------------*/
/*                        End of File                                  */
/*---------------------------------------------------------------------*/

