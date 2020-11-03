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

static void xgb_load(EnigmaticModel_p model)
{
   XGB(XGBoosterCreate(NULL, 0, &model->handle));
   XGB(XGBoosterLoadModel(
      model->handle, 
      model->model_filename
   ));
}

static void xgb_init(EnigmaticWeightXgbParam_p data)
{
   if (data->model1->handle) 
   {
      return;
   }

   int major, minor, patch;
   XGBoostVersion(&major, &minor, &patch);
   fprintf(GlobalOut, "# ENIGMATIC: Using XGBoost version %d.%d.%d\n", major, minor, patch);

   data->load_fun(data->model1);
   EnigmaticInit(data->model1, data->proofstate);
   data->xgb_size = data->model1->vector->features->count;
   if (data->model2)
   {
      data->load_fun(data->model2);
      EnigmaticInit(data->model2, data->proofstate);
      data->xgb_size = MAX(data->xgb_size, data->model2->vector->features->count);
   }
   data->xgb_indices = SizeMalloc(data->xgb_size*sizeof(unsigned int));
   data->xgb_data = SizeMalloc(data->xgb_size*sizeof(float));
   
   fprintf(GlobalOut, "# ENIGMATIC: XGBoost model #1 '%s' loaded with %ld features '%s'\n",
      data->model1->model_filename, 
      data->model1->vector->features->count, 
      DStrView(data->model1->vector->features->spec)
   );
   if (data->model2)
   {
      fprintf(GlobalOut, "# ENIGMATIC: XGBoost model #2 '%s' loaded with %ld features '%s'\n",
         data->model2->model_filename, 
         data->model2->vector->features->count, 
         DStrView(data->model2->vector->features->spec)
      );
   }
}

static void xgb_fill(void* data, long idx, float val)
{
   EnigmaticWeightXgbParam_p local = data;
   if ((!val) || (isnan(val))) { return; }

   local->xgb_indices[local->xgb_count] = idx;
   local->xgb_data[local->xgb_count] = val;
   local->xgb_count++;
}

static double xgb_predict(void* data, EnigmaticModel_p model)
{
   EnigmaticWeightXgbParam_p local = data;
   
   size_t xgb_nelem = local->xgb_count;
   size_t xgb_num_col = model->vector->features->count;
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
      (BoosterHandle)model->handle, 
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

   res->model1 = NULL;
   res->model2 = NULL;
   res->xgb_indices = NULL;
   res->xgb_data = NULL;
   res->fill_fun = xgb_fill;
   res->predict_fun = xgb_predict;
   res->load_fun = xgb_load;

   return res;
}

void EnigmaticWeightXgbParamFree(EnigmaticWeightXgbParam_p junk)
{
   if (junk->xgb_indices)
   {
      SizeFree(junk->xgb_indices, junk->xgb_size*sizeof(unsigned int));
   }
   if (junk->xgb_data)
   {
      SizeFree(junk->xgb_data, junk->xgb_size*sizeof(float));
   }
   if (junk->model1)
   {
      if (junk->model1->handle) 
      {
         XGB(XGBoosterFree((BoosterHandle)junk->model1->handle));
      }
      EnigmaticModelFree(junk->model1);
   }
   if (junk->model2)
   {
      if (junk->model2->handle) 
      {
         XGB(XGBoosterFree((BoosterHandle)junk->model2->handle));
      }
      EnigmaticModelFree(junk->model2);
   }

   EnigmaticWeightXgbParamCellFree(junk);
}
 
WFCB_p EnigmaticWeightXgbParse(
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
   model1 = EnigmaticWeightParse(in, "model.xgb");
   if (TestInpTok(in, Comma))
   {
      if (!model1->weight_type) 
      {
         Error("ENIGMATIC: In the two-phases evaluation, the first model must have binary weights (1)!", USAGE_ERROR);
      }
      NextToken(in);
      model2 = EnigmaticWeightParse(in, "model.xgb");
   }
   AcceptInpTok(in, CloseBracket);

   return EnigmaticWeightXgbInit(
      prio_fun, 
      ocb,
      state,
      model1,
      model2);
}

WFCB_p EnigmaticWeightXgbInit(
   ClausePrioFun prio_fun, 
   OCB_p ocb,
   ProofState_p proofstate,
   EnigmaticModel_p model1,
   EnigmaticModel_p model2)
{
   EnigmaticWeightXgbParam_p data = EnigmaticWeightXgbParamAlloc();
   data->init_fun = xgb_init;
   data->ocb = ocb;
   data->proofstate = proofstate;
   data->model1 = model1;
   data->model2 = model2;

   return WFCBAlloc(
      EnigmaticWeightXgbCompute, 
      prio_fun,
      EnigmaticWeightXgbExit, 
      data);
}

double EnigmaticPredictXgb(Clause_p clause, EnigmaticWeightXgbParam_p local, EnigmaticModel_p model)
{
   if (!model)
   {
      model = local->model1;
   }
   local->xgb_count = 0;
   return EnigmaticPredict(clause, model, local, local->fill_fun, local->predict_fun);
}

double EnigmaticWeightXgbCompute(void* data, Clause_p clause)
{
   EnigmaticWeightXgbParam_p local = data;
   double pred, res;

   local->init_fun(data);
   pred = EnigmaticPredictXgb(clause, local, local->model1);
   res = EnigmaticWeight(pred, local->model1->weight_type, local->model1->threshold);
   if (local->model2)
   {
      if (res == EW_POS)
      {
         pred = EnigmaticPredictXgb(clause, local, local->model2);
         res = EnigmaticWeight(pred, local->model2->weight_type, local->model2->threshold);
      }
      else
      {
         res = EW_WORST;
      }
   }

   if (OutputLevel>=1) 
   {
      fprintf(GlobalOut, "=%.2f (val=%.3f,vlen=%d) : ", res, pred, local->xgb_count);
      ClausePrint(GlobalOut, clause, true);
      fprintf(GlobalOut, "\n");
      if (OutputLevel>=2)
      {
         PrintEnigmaticVector(GlobalOut, local->model1->vector);
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

