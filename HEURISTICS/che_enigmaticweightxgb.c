/*-----------------------------------------------------------------------

File  : che_enigmaticweightxgb.c

Author: Stephan Schultz, AI4REASON

Contents
 
  Copyright 2020 by the authors.
  This code is released under the GNU General Public Licence.
  See the file COPYING in the main CLIB directory for details.
  Run "eprover -h" for contact information.

Changes

<1> Fri 10 Apr 2020 11:14:30 PM CEST

-----------------------------------------------------------------------*/

#include "che_enigmaticweightxgb.h"


/*---------------------------------------------------------------------*/
/*                        Global Variables                             */
/*---------------------------------------------------------------------*/

#define XGB(call) {  \
  int err = (call); \
  if (err != 0) { \
    Error("ENIGMATIC: %s:%d: error in %s: %s\n", OTHER_ERROR, __FILE__, __LINE__, #call, XGBGetLastError());  \
  } \
}

/*---------------------------------------------------------------------*/
/*                      Forward Declarations                           */
/*---------------------------------------------------------------------*/

/*---------------------------------------------------------------------*/
/*                         Internal Functions                          */
/*---------------------------------------------------------------------*/

static void xgb_load(EnigmaticWeightXgbParam_p data)
{
   int major, minor, patch;
   XGBoostVersion(&major, &minor, &patch);
   fprintf(GlobalOut, "# ENIGMATIC: Using XGBoost version %d.%d.%d\n", major, minor, patch);

   XGB(XGBoosterCreate(NULL, 0, &data->xgboost_model));
   XGB(XGBoosterLoadModel(data->xgboost_model, data->model_filename));
}

static void xgb_init(EnigmaticWeightXgbParam_p data)
{
   if (data->xgboost_model) 
   {
      return;
   }

   xgb_load(data);
   
   EnigmaticInitEval(data->features_filename, &data->info, &data->vector, data->proofstate);
   EnigmaticInitProblem(data->vector, data->info, data->proofstate->f_axioms, data->proofstate->axioms);

   data->xgb_indices = SizeMalloc(data->vector->features->count*sizeof(unsigned int));
   data->xgb_data = SizeMalloc(data->vector->features->count*sizeof(float));
   
   //XGBoosterSetAttr(data->xgboost_model, "objective", "binary:logistic");
   //XGBoosterSetParam(data->xgboost_model, "num_feature", "4096");
   
   fprintf(GlobalOut, "# ENIGMATIC: XGBoost model '%s' loaded with %ld features '%s'\n",
         data->model_filename, data->vector->features->count, 
         DStrView(data->vector->features->spec));
}

static void xgb_fill(void* data, long idx, float val)
{
   EnigmaticWeightXgbParam_p local = data;
   if ((!val) || (isnan(val))) { return; }

   local->xgb_indices[local->xgb_count] = idx;
   local->xgb_data[local->xgb_count] = val;
   local->xgb_count++;
}

static double xgb_predict(void* data)
{
   EnigmaticWeightXgbParam_p local = data;
   
   size_t xgb_nelem = local->xgb_count;
   size_t xgb_num_col = local->vector->features->count;
   size_t xgb_nindptr = 2;
   static bst_ulong xgb_indptr[2];
   xgb_indptr[0] = 0L;
   xgb_indptr[1] = xgb_nelem;
   DMatrixHandle xgb_matrix = NULL;
   XGB(XGDMatrixCreateFromCSREx(
      xgb_indptr, 
      local->xgb_indices, 
      local->xgb_data, 
      xgb_nindptr, 
      xgb_nelem, 
      xgb_num_col, 
      &xgb_matrix
   ));

   bst_ulong out_len = 0L;
   const float* pred;
   XGB(XGBoosterPredict(
      local->xgboost_model, 
      xgb_matrix, 
      0, 
      0, 
      0, 
      &out_len, 
      &pred
   ));

   XGB(XGDMatrixFree(xgb_matrix));

   return pred[0];
}

/*---------------------------------------------------------------------*/
/*                         Exported Functions                          */
/*---------------------------------------------------------------------*/

EnigmaticWeightXgbParam_p EnigmaticWeightXgbParamAlloc(void)
{
   EnigmaticWeightXgbParam_p res = EnigmaticWeightXgbParamCellAlloc();

   res->xgboost_model = NULL;
   res->vector = NULL;
   res->info = NULL;
   res->xgb_indices = NULL;
   res->xgb_data = NULL;

   return res;
}

void EnigmaticWeightXgbParamFree(EnigmaticWeightXgbParam_p junk)
{
   if (junk->xgb_indices)
   {
      SizeFree(junk->xgb_indices, junk->vector->features->count*sizeof(unsigned int));
   }
   if (junk->xgb_data)
   {
      SizeFree(junk->xgb_data, junk->vector->features->count*sizeof(float));
   }
   if (junk->xgboost_model)
   {
      XGBoosterFree(junk->xgboost_model);
   }
   if (junk->vector)
   {
      EnigmaticVectorFree(junk->vector);
   }
   if (junk->info)
   {
      EnigmaticInfoFree(junk->info);
   }

   EnigmaticWeightXgbParamCellFree(junk);
}
 
WFCB_p EnigmaticWeightXgbParse(
   Scanner_p in,  
   OCB_p ocb, 
   ProofState_p state)
{   
   ClausePrioFun prio_fun;
   char* model_filename;
   char* features_filename;
   int binary_weights;
   double threshold;

   AcceptInpTok(in, OpenBracket);
   prio_fun = ParsePrioFun(in);
   AcceptInpTok(in, Comma);

   EnigmaticWeightParse(in, &model_filename, &features_filename, &binary_weights, &threshold, "model.xgb");

   return EnigmaticWeightXgbInit(
      prio_fun, 
      ocb,
      state,
      model_filename,
      features_filename,
      binary_weights,
      threshold);
}

WFCB_p EnigmaticWeightXgbInit(
   ClausePrioFun prio_fun, 
   OCB_p ocb,
   ProofState_p proofstate,
   char* model_filename,
   char* features_filename,
   int binary_weights,
   double threshold)
{
   EnigmaticWeightXgbParam_p data = EnigmaticWeightXgbParamAlloc();

   data->init_fun = xgb_init;
   data->ocb = ocb;
   data->proofstate = proofstate;
   
   data->model_filename = model_filename;
   data->features_filename = features_filename;
   data->binary_weights = binary_weights;
   data->threshold = threshold;
   
   return WFCBAlloc(
      EnigmaticWeightXgbCompute, 
      prio_fun,
      EnigmaticWeightXgbExit, 
      data);
}

double EnigmaticWeightXgbCompute(void* data, Clause_p clause)
{
   EnigmaticWeightXgbParam_p local = data;
   double pred, res;

   local->init_fun(data);
   local->xgb_count = 0;
   //pred = EnigmaticPredict(clause, local->vector, local->info, xgb_fill, xgb_predict, data);
   EnigmaticClauseReset(local->vector->clause);
   EnigmaticClause(local->vector->clause, clause, local->info);
   EnigmaticVectorFill(local->vector, xgb_fill, data);
   pred = xgb_predict(data);
   res = EnigmaticWeight(pred, local->binary_weights, local->threshold);

   /*
   EnigmaticClause(local->vector->clause, clause, local->info);
   EnigmaticVectorFill(local->vector, xgb_fill, local);
   EnigmaticClauseReset(local->vector->clause);

   double pred = xgb_predict(data);
  
   if (local->binary_weights)
   {
      if (pred <= local->threshold) { res = EW_NEG; } else { res = EW_POS; }
   }
   else
   {
      res = 1 + ((EW_POS - pred) * EW_NEG);
   }
   */

   if (OutputLevel>=1) 
   {
      fprintf(GlobalOut, "=%.2f (val=%.3f,vlen=%d) : ", res, pred, local->xgb_count);
      ClausePrint(GlobalOut, clause, true);
      fprintf(GlobalOut, "\n");
      if (OutputLevel>=2)
      {
         PrintEnigmaticVector(GlobalOut, local->vector);
         fprintf(GlobalOut, "\n");
      }
   }
   
   return res;
}

void EnigmaticWeightXgbExit(void* data)
{
   EnigmaticWeightXgbParam_p junk = data;
   EnigmaticWeightXgbParamFree(junk);
}

/*---------------------------------------------------------------------*/
/*                        End of File                                  */
/*---------------------------------------------------------------------*/

