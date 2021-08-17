/*-----------------------------------------------------------------------

File  : che_enigmaticgenerationlgb.c

Author: Stephan Schultz, AI4REASON

Contents
 
  Copyright 2020 by the authors.
  This code is released under the GNU General Public Licence.
  See the file COPYING in the main CLIB directory for details.
  Run "eprover -h" for contact information.

Changes

<1> Fri 10 Apr 2020 11:14:30 PM CEST

-----------------------------------------------------------------------*/

#include "che_enigmaticgenerationlgb.h"


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

static void lgb_init(EnigmaticGenerationLgbParam_p data)
{
   if (data->model1->handle) 
   {
      return;
   }

   data->load_fun(data->model1);
   EnigmaticInit(data->model1, data->proofstate);
   data->lgb_size = data->model1->vector->features->count;
   data->concat = data->model1->vector->features->co_parent;

   data->lgb_indices = SizeMalloc(data->lgb_size*sizeof(int32_t));
   data->lgb_data = SizeMalloc(data->lgb_size*sizeof(float));
   
   fprintf(GlobalOut, "# ENIGMATIC: LightGBM parental guidance model #1 '%s' loaded with %ld features '%s'\n",
      data->model1->model_filename, 
      data->model1->vector->features->count, 
      DStrView(data->model1->vector->features->spec)
   );
}

static void lgb_fill(void* data, long idx, float val)
{
   EnigmaticGenerationLgbParam_p local = data;
   if ((!val) || (isnan(val))) { return; }

   local->lgb_indices[local->lgb_count] = idx;
   local->lgb_data[local->lgb_count] = val;
   local->lgb_count++;
}

static double lgb_predict(void* data, EnigmaticModel_p model)
{
   EnigmaticGenerationLgbParam_p local = data;

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

EnigmaticGenerationLgbParam_p EnigmaticGenerationLgbParamAlloc(void)
{
   EnigmaticGenerationLgbParam_p res = EnigmaticGenerationLgbParamCellAlloc();

   res->model1 = NULL;
   res->lgb_indices = NULL;
   res->lgb_data = NULL;
   res->fill_fun = lgb_fill;
   res->predict_fun = lgb_predict;
   res->load_fun = lgb_load;

   return res;
}

void EnigmaticGenerationLgbParamFree(EnigmaticGenerationLgbParam_p junk)
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
   EnigmaticGenerationLgbParamCellFree(junk);
}
 
void EnigmaticGenerationLgbModelInit(
   char* d_prefix,
   char* model_name,
   double threshold,
   OCB_p ocb,
   ProofState_p state,
   EnigmaticGenerationLgbParam_p data)
{   
   EnigmaticModel_p model1;

   model1 = EnigmaticModelCreate(d_prefix, model_name); // "model.lgb"
   model1->weight_type = 1; // res = (pred <= threshold) ? EW_NEG : EW_POS;
   model1->threshold = threshold;

   data->ocb = ocb;
   data->proofstate = state;
   data->init_fun = lgb_init;
   data->model1 = model1;
}


double EnigmaticGenerationPredictLgb(Clause_p clause, EnigmaticGenerationLgbParam_p local, EnigmaticModel_p model)
{
   if (!model)
   {
      model = local->model1;
   }
   local->lgb_count = 0;
   return EnigmaticPredict(clause, model, local, local->fill_fun, local->predict_fun);
}

double EnigmaticGenerationPredictParentsLgb(Clause_p parent1, Clause_p parent2, EnigmaticGenerationLgbParam_p local, EnigmaticModel_p model)
{
   if (!model)
   {
      model = local->model1;
   }
   local->lgb_count = 0;

   return EnigmaticPredictParents(parent1, parent2, model, local, local->fill_fun, local->predict_fun);
}

double EnigmaticGenerationPredictParentsConcatLgb(Clause_p parent1, Clause_p parent2, EnigmaticGenerationLgbParam_p local, EnigmaticModel_p model)
{
   if (!model)
   {
      model = local->model1;
   }
   local->lgb_count = 0;

   return EnigmaticPredictParentsConcat(parent1, parent2, model, local, local->fill_fun, local->predict_fun);
}

bool EnigmaticLgbFilterGenerationCompute(EnigmaticGenerationLgbParam_p local, Clause_p clause)
{
	double pred = 0.0;
	bool res = false;

	PStackPointer j = 0;
	DerivationCode op;
	Clause_p parent1 = NULL;
	Clause_p parent2 = NULL;

	local->init_fun(local);

	op = PStackElementInt(clause->derivation, j);
	j++;

	if(DCOpHasCnfArg1(op))
	{
	   parent1 = PStackElementP(clause->derivation, j);
	   j++;
	   if (OutputLevel>=1)
	   	{
		   fprintf(GlobalOut, " #parent1");
		   ClausePrint(GlobalOut, parent1, true);
		   fprintf(GlobalOut, " ");
	   	}
	}
	if(DCOpHasCnfArg2(op))
	{
	   parent2 = PStackElementP(clause->derivation, j);
	   j++;
	   if (OutputLevel>=1)
	   	{
		   fprintf(GlobalOut, " #parent2");
		   ClausePrint(GlobalOut, parent2, true);
		   fprintf(GlobalOut, " ");
		   fprintf(GlobalOut, "\n");
	   	}
	}

	// The models are trained only based on pairs of parents.
	if (parent1 && parent2)
	{
		if (local->concat)
		{
			pred = EnigmaticGenerationPredictParentsConcatLgb(parent1, parent2, local, local->model1);
		}
		else
		{
			pred = EnigmaticGenerationPredictParentsLgb(parent1, parent2, local, local->model1);
		}
		res = (pred <= local->model1->threshold);
	}
	if (OutputLevel>=1)
	{
	  if (OutputLevel>=2)
	  {
		 fprintf(GlobalOut, "+? ");
		 PrintEnigmaticVector(GlobalOut, local->model1->vector);
		 fprintf(GlobalOut, "\n");
	  }
	  fprintf(GlobalOut, "=%s (val=%.3f,vlen=%d) : ", res ? "true" : "false", pred, local->lgb_count);
	  ClausePrint(GlobalOut, clause, true);
	  fprintf(GlobalOut, "\n");
	}

	return res;
}

void EnigmaticGenerationLgbExit(void* data)
{
   EnigmaticGenerationLgbParam_p junk = data;
   EnigmaticGenerationLgbParamFree(junk);
}

/*---------------------------------------------------------------------*/
/*                        End of File                                  */
/*---------------------------------------------------------------------*/

