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

static void lgb_load(EnigmaticModel_p model)
{
   int iters;
   LGB(LGBM_BoosterCreateFromModelfile(
      model->model_filename, 
      &iters, 
      &model->handle
   ));
}

static void lgb_init(EnigmaticWeightLgbParam_p data)
{
   if (data->model1->handle) 
   {
      return;
   }

   data->load_fun(data->model1);
   EnigmaticInit(data->model1, data->proofstate);
   data->lgb_size = data->model1->vector->features->count;
   if (data->model2)
   {
      data->load_fun(data->model2);
      EnigmaticInit(data->model2, data->proofstate);
      data->lgb_size = MAX(data->lgb_size, data->model2->vector->features->count);
   }
   data->lgb_indices = SizeMalloc(data->lgb_size*sizeof(int32_t));
   data->lgb_data = SizeMalloc(data->lgb_size*sizeof(float));
   
   fprintf(GlobalOut, "# ENIGMATIC: LightGBM model #1 '%s' loaded with %ld features '%s'\n",
      data->model1->model_filename, 
      data->model1->vector->features->count, 
      DStrView(data->model1->vector->features->spec)
   );
   if (data->model2)
   {
      fprintf(GlobalOut, "# ENIGMATIC: LightGBM model #2 '%s' loaded with %ld features '%s'\n",
         data->model2->model_filename, 
         data->model2->vector->features->count, 
         DStrView(data->model1->vector->features->spec)
      );
   }
}

static void lgb_fill(void* data, long idx, float val)
{
   EnigmaticWeightLgbParam_p local = data;
   if ((!val) || (isnan(val))) { return; }

   local->lgb_indices[local->lgb_count] = idx;
   local->lgb_data[local->lgb_count] = val;
   local->lgb_count++;
}

static double lgb_predict(void* data, EnigmaticModel_p model)
{
   EnigmaticWeightLgbParam_p local = data;

   int64_t lgb_nelem = local->lgb_count;
   int64_t lgb_num_col = model->vector->features->count;
   int64_t lgb_nindptr = 2;
   static int32_t lgb_indptr[2];
   lgb_indptr[0] = 0;
   lgb_indptr[1] = lgb_nelem;

   int64_t out_len = 0L;
   double pred[2];
   LGB(LGBM_BoosterPredictForCSRSingleRow(
      model->handle, 
      lgb_indptr, 
      C_API_DTYPE_INT32,
      local->lgb_indices, 
      local->lgb_data, 
      C_API_DTYPE_FLOAT32, 
      lgb_nindptr, 
      lgb_nelem,
      lgb_num_col, 
      C_API_PREDICT_NORMAL, 
      0, 
      0, 
      "", 
      &out_len, 
      pred
   ));

   return pred[0];
}

/*---------------------------------------------------------------------*/
/*                         Exported Functions                          */
/*---------------------------------------------------------------------*/

EnigmaticWeightLgbParam_p EnigmaticWeightLgbParamAlloc(void)
{
   EnigmaticWeightLgbParam_p res = EnigmaticWeightLgbParamCellAlloc();

   res->model1 = NULL;
   res->model2 = NULL;
   res->lgb_indices = NULL;
   res->lgb_data = NULL;
   res->fill_fun = lgb_fill;
   res->predict_fun = lgb_predict;
   res->load_fun = lgb_load;

   return res;
}

void EnigmaticWeightLgbParamFree(EnigmaticWeightLgbParam_p junk)
{
   if (junk->lgb_indices)
   {
      SizeFree(junk->lgb_indices, junk->lgb_size*sizeof(int32_t));
   }
   if (junk->lgb_data)
   {
      SizeFree(junk->lgb_data, junk->lgb_size*sizeof(float));
   }
   if (junk->model1)
   {
      if (junk->model1->handle)
      {
         LGB(LGBM_BoosterFree((BoosterHandle)junk->model1->handle));
      }
      EnigmaticModelFree(junk->model1);
   }
   if (junk->model2)
   {
      if (junk->model2->handle)
      {
         LGB(LGBM_BoosterFree((BoosterHandle)junk->model2->handle));
      }
      EnigmaticModelFree(junk->model2);
   }

   EnigmaticWeightLgbParamCellFree(junk);
}
 
WFCB_p EnigmaticWeightLgbParse(
   Scanner_p in,  
   OCB_p ocb, 
   ProofState_p state)
{   
   EnigmaticModel_p model1;
   EnigmaticModel_p model2 = NULL;
   ClausePrioFun prio_fun;

   AcceptInpTok(in, OpenBracket);
   prio_fun = ParsePrioFun(in);
   AcceptInpTok(in, Comma);
   model1 = EnigmaticWeightParse(in, "model.lgb");
   if (TestInpTok(in, Comma))
   {
      if (model1->weight_type != 1) 
      {
         Error("ENIGMATIC: In the two-phases evaluation, the first model must have binary weight type (1)!", USAGE_ERROR);
      }
      NextToken(in);
      model2 = EnigmaticWeightParse(in, "model.lgb");
   }
   AcceptInpTok(in, CloseBracket);

   return EnigmaticWeightLgbInit(
      prio_fun, 
      ocb,
      state,
      model1,
      model2);
}

EnigmaticWeightLgbParam_p EnigmaticWeightInit(
   OCB_p ocb,
   ProofState_p proofstate,
   EnigmaticModel_p model1,
   EnigmaticModel_p model2)
{
   EnigmaticWeightLgbParam_p data = EnigmaticWeightLgbParamAlloc();
   data->init_fun = lgb_init;
   data->ocb = ocb;
   data->proofstate = proofstate;
   data->model1 = model1;
   data->model2 = model2;
   return data;
}

WFCB_p EnigmaticWeightLgbInit(
   ClausePrioFun prio_fun, 
   OCB_p ocb,
   ProofState_p proofstate,
   EnigmaticModel_p model1,
   EnigmaticModel_p model2)
{
   EnigmaticWeightLgbParam_p data;
   data = EnigmaticWeightInit(ocb, proofstate, model1, model2);
   return WFCBAlloc(
      EnigmaticWeightLgbCompute, 
      prio_fun,
      EnigmaticWeightLgbExit, 
      data);
}

double EnigmaticPredictLgb(Clause_p clause, EnigmaticWeightLgbParam_p local, EnigmaticModel_p model)
{
   if (!model)
   {
      model = local->model1;
   }
   local->lgb_count = 0;
   return EnigmaticPredict(clause, model, local, local->fill_fun, local->predict_fun);
}

double EnigmaticWeightLgbCompute(void* data, Clause_p clause)
{
   EnigmaticWeightLgbParam_p local = data;
   double pred, res;

   local->init_fun(data);
   pred = EnigmaticPredictLgb(clause, local, local->model1);
   res = EnigmaticWeight(pred, local->model1->weight_type, local->model1->threshold);
   if (local->model2)
   {
      if (res == EW_POS)
      {
         pred = EnigmaticPredictLgb(clause, local, local->model2);
         res = EnigmaticWeight(pred, local->model2->weight_type, local->model2->threshold);
      }
      else
      {
         res = EW_WORST;
      }
   }

   if (OutputLevel>=1) 
   {
      if (OutputLevel>=2)
      {
         fprintf(GlobalOut, "+? ");
         PrintEnigmaticVector(GlobalOut, local->model1->vector);
         fprintf(GlobalOut, "\n");
      }
      fprintf(GlobalOut, "=%.2f (val=%.3f,vlen=%d) : ", res, pred, local->lgb_count);
      ClausePrint(GlobalOut, clause, true);
      fprintf(GlobalOut, "\n");
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

