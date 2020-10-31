/*-----------------------------------------------------------------------

File  : che_enigmaticweightlgb.c

Author: Stephan Schultz, AI4REASON

Contents
 
  Copyright 2020 by the authors.
  This code is released under the GNU General Public Licence.
  See the file COPYING in the main CLIB directory for details.
  Run "eprover -h" for contact information.

Changes

<1> Fri 10 Apr 2020 11:14:30 PM CEST

-----------------------------------------------------------------------*/

#include "che_enigmaticweightlgb.h"


/*---------------------------------------------------------------------*/
/*                        Global Variables                             */
/*---------------------------------------------------------------------*/

#define LGB(call) {  \
  int err = (call); \
  if (err != 0) { \
    Error("ENIGMATIC: %s:%d: error in %s: %s\n", OTHER_ERROR, __FILE__, __LINE__, #call, LGBM_GetLastError());  \
  } \
}

/*---------------------------------------------------------------------*/
/*                      Forward Declarations                           */
/*---------------------------------------------------------------------*/

/*---------------------------------------------------------------------*/
/*                         Internal Functions                          */
/*---------------------------------------------------------------------*/

static void lgb_load(EnigmaticWeightLgbParam_p data)
{

   int iters;
   LGB(LGBM_BoosterCreateFromModelfile(data->model_filename, &iters, &data->lgb_model));
}

static void lgb_init(EnigmaticWeightLgbParam_p data)
{
   if (data->lgb_model) 
   {
      return;
   }

   lgb_load(data);
   
   EnigmaticInitEval(data->features_filename, &data->info, &data->vector, data->proofstate);
   EnigmaticInitProblem(data->vector, data->info, data->proofstate->f_axioms, data->proofstate->axioms);

   data->lgb_indices = SizeMalloc(data->vector->features->count*sizeof(int32_t));
   data->lgb_data = SizeMalloc(data->vector->features->count*sizeof(float));
   
   fprintf(GlobalOut, "# ENIGMATIC: LightGBM model '%s' loaded with %ld features '%s'\n",
         data->model_filename, data->vector->features->count, 
         DStrView(data->vector->features->spec));
}

static void lgb_fill(void* data, long idx, float val)
{
   EnigmaticWeightLgbParam_p local = data;
   if ((!val) || (isnan(val))) { return; }

   local->lgb_indices[local->lgb_count] = idx;
   local->lgb_data[local->lgb_count] = val;
   local->lgb_count++;
}

static double lgb_predict(void* data)
{
   EnigmaticWeightLgbParam_p local = data;

   int64_t lgb_nelem = local->lgb_count;
   int64_t lgb_num_col = local->vector->features->count;
   int64_t lgb_nindptr = 2;
   static int32_t lgb_indptr[2];
   lgb_indptr[0] = 0;
   lgb_indptr[1] = lgb_nelem;

   int64_t out_len = 0L;
   double pred[2];
   LGB(LGBM_BoosterPredictForCSRSingleRow(local->lgb_model, lgb_indptr, C_API_DTYPE_INT32,
      local->lgb_indices, local->lgb_data, C_API_DTYPE_FLOAT32, lgb_nindptr, lgb_nelem,
      lgb_num_col, C_API_PREDICT_NORMAL, 0, 0, "", &out_len, pred));

   return pred[0];
}

/*---------------------------------------------------------------------*/
/*                         Exported Functions                          */
/*---------------------------------------------------------------------*/

EnigmaticWeightLgbParam_p EnigmaticWeightLgbParamAlloc(void)
{
   EnigmaticWeightLgbParam_p res = EnigmaticWeightLgbParamCellAlloc();

   res->lgb_model = NULL;
   res->vector = NULL;
   res->info = NULL;
   res->lgb_indices = NULL;
   res->lgb_data = NULL;

   return res;
}

void EnigmaticWeightLgbParamFree(EnigmaticWeightLgbParam_p junk)
{
   if (junk->lgb_indices)
   {
      SizeFree(junk->lgb_indices, junk->vector->features->count*sizeof(unsigned int));
   }
   if (junk->lgb_data)
   {
      SizeFree(junk->lgb_data, junk->vector->features->count*sizeof(float));
   }
   if (junk->lgb_model)
   {
      LGB(LGBM_BoosterFree(junk->lgb_model));
   }
   if (junk->vector)
   {
      EnigmaticVectorFree(junk->vector);
   }
   if (junk->info)
   {
      EnigmaticInfoFree(junk->info);
   }

   EnigmaticWeightLgbParamCellFree(junk);
}
 
WFCB_p EnigmaticWeightLgbParse(
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

   EnigmaticWeightParse(in, &model_filename, &features_filename, &binary_weights, &threshold, "model.lgb");

   return EnigmaticWeightLgbInit(
      prio_fun, 
      ocb,
      state,
      model_filename,
      features_filename,
      binary_weights,
      threshold);
}

WFCB_p EnigmaticWeightLgbInit(
   ClausePrioFun prio_fun, 
   OCB_p ocb,
   ProofState_p proofstate,
   char* model_filename,
   char* features_filename,
   int binary_weights,
   double threshold)
{
   EnigmaticWeightLgbParam_p data = EnigmaticWeightLgbParamAlloc();

   data->init_fun = lgb_init;
   data->ocb = ocb;
   data->proofstate = proofstate;
   
   data->model_filename = model_filename;
   data->features_filename = features_filename;
   data->binary_weights = binary_weights;
   data->threshold = threshold;
   
   return WFCBAlloc(
      EnigmaticWeightLgbCompute, 
      prio_fun,
      EnigmaticWeightLgbExit, 
      data);
}

double EnigmaticWeightLgbCompute(void* data, Clause_p clause)
{
   EnigmaticWeightLgbParam_p local = data;
   double pred, res;

   local->init_fun(data);
   local->lgb_count = 0;
   EnigmaticClauseReset(local->vector->clause);
   EnigmaticClause(local->vector->clause, clause, local->info);
   EnigmaticVectorFill(local->vector, lgb_fill, data);
   pred = lgb_predict(data);
   res = EnigmaticWeight(pred, local->binary_weights, local->threshold);

   if (OutputLevel>=1) 
   {
      fprintf(GlobalOut, "=%.2f (val=%.3f,vlen=%d) : ", res, pred, local->lgb_count);
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

void EnigmaticWeightLgbExit(void* data)
{
   EnigmaticWeightLgbParam_p junk = data;
   EnigmaticWeightLgbParamFree(junk);
}

/*---------------------------------------------------------------------*/
/*                        End of File                                  */
/*---------------------------------------------------------------------*/

