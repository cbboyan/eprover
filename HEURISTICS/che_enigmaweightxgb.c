/*-----------------------------------------------------------------------

File  : che_enigmaweightxgb.c

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

#include "che_enigmaweightxgb.h"


/*---------------------------------------------------------------------*/
/*                        Global Variables                             */
/*---------------------------------------------------------------------*/

/*---------------------------------------------------------------------*/
/*                      Forward Declarations                           */
/*---------------------------------------------------------------------*/

/*---------------------------------------------------------------------*/
/*                         Internal Functions                          */
/*---------------------------------------------------------------------*/

static unsigned conj_indices[2048]; // TODO: dynamic alloc
static float conj_data[2048]; // TODO: dynamic alloc

static void extweight_init(EnigmaWeightXgbParam_p data)
{
   if (data->xgboost_model) 
   {
      return;
   }
  
   XGBoosterCreate(NULL, 0, &data->xgboost_model);

   if (XGBoosterLoadModel(data->xgboost_model, data->model_filename) != 0)
   {
      Error("ENIGMA: Failed loading XGBoost model '%s':\n%s", FILE_ERROR,
         data->model_filename, XGBGetLastError());
   }
   //XGBoosterSetAttr(data->xgboost_model, "objective", "binary:logistic");

   data->enigmap = EnigmapLoad(data->features_filename, data->ocb->sig);

   // problem features:
   SpecFeature_p spec = SpecFeatureCellAlloc();
   SpecFeaturesCompute(spec, data->proofstate->axioms, data->enigmap->sig);
   EnigmapFillProblemFeatures(data->enigmap, data->proofstate->axioms);
   SpecFeatureCellFree(spec);

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
         }
      }
      FeaturesAddVariables(&features, &varstat, data->enigmap, &len);

      if (len >= 2048) { Error("ENIGMA: Too many conjecture features!", OTHER_ERROR); } 
  
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

   fprintf(GlobalOut, "# ENIGMA: XGBoost model '%s' loaded. (%s: %ld; conj_feats: %d; version: %ld)\n", 
      data->model_filename, 
      (data->enigmap->version & EFHashing) ? "hash_base" : "features", data->enigmap->feature_count, 
      data->conj_features_count, data->enigmap->version);
}

/*---------------------------------------------------------------------*/
/*                         Exported Functions                          */
/*---------------------------------------------------------------------*/

EnigmaWeightXgbParam_p EnigmaWeightXgbParamAlloc(void)
{
   EnigmaWeightXgbParam_p res = EnigmaWeightXgbParamCellAlloc();

   res->xgboost_model = NULL;
   res->enigmap = NULL;

   return res;
}

void EnigmaWeightXgbParamFree(EnigmaWeightXgbParam_p junk)
{
   free(junk->model_filename);
   free(junk->features_filename);
   junk->xgboost_model = NULL; // TODO: free model & enigmap

   EnigmaWeightXgbParamCellFree(junk);
}
 
WFCB_p EnigmaWeightXgbParse(
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
   DStrAppendStr(f_model, "model.xgb");
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

   return EnigmaWeightXgbInit(
      prio_fun, 
      ocb,
      state,
      model_filename,
      features_filename,
      len_mult);
}

WFCB_p EnigmaWeightXgbInit(
   ClausePrioFun prio_fun, 
   OCB_p ocb,
   ProofState_p proofstate,
   char* model_filename,
   char* features_filename,
   double len_mult)
{
   EnigmaWeightXgbParam_p data = EnigmaWeightXgbParamAlloc();

   data->init_fun   = extweight_init;
   data->ocb        = ocb;
   data->proofstate = proofstate;
   
   data->model_filename = model_filename;
   data->features_filename = features_filename;
   data->len_mult = len_mult;
   
   return WFCBAlloc(
      EnigmaWeightXgbCompute, 
      prio_fun,
      EnigmaWeightXgbExit, 
      data);
}

double EnigmaWeightXgbCompute(void* data, Clause_p clause)
{
   static unsigned xgb_indices[2048]; // TODO: dynamic alloc
   static float xgb_data[2048]; // TODO: dynamic alloc
   EnigmaWeightXgbParam_p local;
   double res = 1.0;
   
   local = data;
   local->init_fun(data);
   
   long long start = GetUSecClock();
   int len = 0;
   NumTree_p features = FeaturesClauseCollect(clause, local->enigmap, &len);
   //printf("features count: %d\n", len);
      
   if (len+local->conj_features_count >= 2048) { Error("ENIGMA: Too many clause features!", OTHER_ERROR); }

   int i = 0;
   while (features) 
   {
      NumTree_p cell = NumTreeExtractEntry(&features,NumTreeMinNode(features)->key);
      xgb_indices[i] = cell->key;
      xgb_data[i] = (float)cell->val1.i_val;
      //printf("%d:%.0f ", xgb_indices[i], xgb_data[i]);
      i++;
      NumTreeCellFree(cell);
   }
   //printf("|");

   for (int j=0; j<local->conj_features_count; j++) 
   {
      xgb_indices[i+j] = local->conj_features_indices[j];
      xgb_data[i+j] = local->conj_features_data[j];
      //printf("%d:%.0f ", xgb_indices[i+j], xgb_data[i+j]);
   }
  
   if (local->enigmap->version & EFProblem)
   {
      for (int j=0; j<22; j++)
      {
         xgb_indices[i+local->conj_features_count+j] = (2*local->enigmap->feature_count)+1+j;
         xgb_data   [i+local->conj_features_count+j] = local->enigmap->problem_features[j];
      }
   }
   int total = i+local->conj_features_count+22;
   
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
         xgb_indices[k] = proof->key + offset;
         xgb_data[k] = ((double)proof->val1.i_val)/len->val1.i_val;
         //printf("%d:%.3f ", xgb_indices[k], xgb_data[k]);
         k++;
         if (k >= 2048) { Error("ENIGMA: Too many proof watch features!", OTHER_ERROR); }
      }
      NumTreeTraverseExit(stack);
      total = k;
   }
   //printf("\n");
   //printf("[duration] feature extract: %f.2 ms\n", (double)(GetUSecClock() - start)/1000.0);
   
   //start = clock();
   size_t xgb_nelem = total; //i + local->conj_features_count;
   size_t xgb_num_col = 1 + local->enigmap->feature_count +
      (local->enigmap->version & EFConjecture ? local->enigmap->feature_count : 0) +
      (local->enigmap->version & EFProblem ? 22 : 0) +
      (local->enigmap->version & EFProofWatch ? local->proofstate->wlcontrol->proofs_count : 0);
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
   //printf("[duration] xgb matrix: %f.2 ms\n", (double)(clock() - start)/ (CLOCKS_PER_SEC / 1000));

   //start = clock();
   bst_ulong out_len = 0L;
   const float* pred;
   if (XGBoosterPredict(local->xgboost_model, xgb_matrix, 
          0, 0, &out_len, &pred) != 0)
   {
      Error("ENIGMA: Failed computing XGBoost prediction:\n%s", 
         OTHER_ERROR, XGBGetLastError());
   }
   printf("prediction: %p\n", pred);
   //printf("prediction: len=%ld first=%f\n", out_len, pred[0]);
   
   //res = 1 + ((1.0 - pred[0]) * 10.0);
   if (pred[0] <= 0.5) { res = 10.0; } else { res = 1.0; }

   XGDMatrixFree(xgb_matrix);
   /*
   res = predict(local->linear_model, nodes);
   //fprintf(GlobalOut, "+%0.2f ", res);
   */
   
   double clen = ClauseWeight(clause,1,1,1,1,1,false);
   res = (clen * local->len_mult) + res;

   if (OutputLevel>=1) {
      fprintf(GlobalOut, "=%.2f (val=%.3f,t=%.3fms,clen=%.1f,vlen=%ld) : ", res, pred[0], (double)(GetUSecClock() - start)/ 1000.0, clen, xgb_nelem);
      ClausePrint(GlobalOut, clause, true);
      fprintf(GlobalOut, "\n");
   }
   
   //printf("[duration] xgb predict: %.3f ms   (clen=%.1f, vlen=%ld)\n", (double)(GetUSecClock() - start)/ 1000.0, clen, xgb_nelem);

   return res;
}

void EnigmaWeightXgbExit(void* data)
{
   EnigmaWeightXgbParam_p junk = data;
   
   EnigmaWeightXgbParamFree(junk);
}

/*---------------------------------------------------------------------*/
/*                        End of File                                  */
/*---------------------------------------------------------------------*/

