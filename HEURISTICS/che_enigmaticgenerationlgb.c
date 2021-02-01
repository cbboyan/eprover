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
   //if (data->model2)
   //{
   //   data->load_fun(data->model2);
   //   EnigmaticInit(data->model2, data->proofstate);
   //   data->lgb_size = MAX(data->lgb_size, data->model2->vector->features->count);
   //}
   data->lgb_indices = SizeMalloc(data->lgb_size*sizeof(int32_t));
   data->lgb_data = SizeMalloc(data->lgb_size*sizeof(float));
   
   fprintf(GlobalOut, "# ENIGMATIC: LightGBM model #1 '%s' loaded with %ld features '%s'\n",
      data->model1->model_filename, 
      data->model1->vector->features->count, 
      DStrView(data->model1->vector->features->spec)
   );
   //if (data->model2)
   //{
   //   fprintf(GlobalOut, "# ENIGMATIC: LightGBM model #2 '%s' loaded with %ld features '%s'\n",
   //      data->model2->model_filename,
   //      data->model2->vector->features->count,
   //      DStrView(data->model1->vector->features->spec)
   //   );
   //}
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
   //res->model2 = NULL;
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
   //if (junk->model2)
   //{
   //   if (junk->model2->handle)
   //   {
   //      LGB(LGBM_BoosterFree((BoosterHandle)junk->model2->handle));
   //   }
   //   EnigmaticModelFree(junk->model2);
   //}

   EnigmaticGenerationLgbParamCellFree(junk);
}
 
//WFCB_p EnigmaticGenerationLgbParse(
// We much want to skip the return and just add the model to the ProofControl cell, actually...
//EnigmaticGenerationLgbParam_p EnigmaticGenerationLgbModelInit(
void EnigmaticGenerationLgbModelInit(
   //Scanner_p in,  // As the d_prefix is now given as a string input, we no longer need the stream!
   char* d_prefix,
   char* model_name,
   OCB_p ocb,
   ProofState_p state,
   EnigmaticGenerationLgbParam_p data)
{   
   EnigmaticModel_p model1;

   model1 = EnigmaticModelCreate(d_prefix, model_name);// "model.lgb");
   model1->weight_type = 1; // res = (pred <= threshold) ? EW_NEG : EW_POS;
   model1->threshold = 0.5;

   //Now done in the ProofControl Allocation
   //EnigmaticGenerationLgbParam_p data = EnigmaticGenerationLgbParamAlloc();
   // Can't do this because it's not defined yet.  But returning seems crude and ugly.
   //EnigmaticGenerationLgbParam_p data = control->enigma_gen_model;

   // Fairly straightforward
   data->ocb = ocb;
   data->proofstate = state;
   data->init_fun = lgb_init;
   data->model1 = model1;

   //EnigmaticModel_p model2 = NULL;
   //ClausePrioFun prio_fun;

   //AcceptInpTok(in, OpenBracket);
   //prio_fun = ParsePrioFun(in);
   //AcceptInpTok(in, Comma);
   //model1 = EnigmaticGenerationParse(in, "model.lgb");
   //if (TestInpTok(in, Comma))
   //{
   //   if (model1->weight_type != 1)
   //   {
   //      Error("ENIGMATIC: In the two-phases evaluation, the first model must have binary weight type (1)!", USAGE_ERROR);
   //   }
   //   NextToken(in);
   //   model2 = EnigmaticGenerationParse(in, "model.lgb");
   //}
   //AcceptInpTok(in, CloseBracket);

   //return data;
   //return EnigmaticGenerationLgbInit(
   //   prio_fun,
   //   ocb,
   //   state,
   //   model1);//,
      //model2);
}

//WFCB_p EnigmaticGenerationLgbInit(
//   ClausePrioFun prio_fun,
//   OCB_p ocb,
//   ProofState_p proofstate,
//   EnigmaticModel_p model1)//,
//   //EnigmaticModel_p model2)
//{
//   EnigmaticGenerationLgbParam_p data = EnigmaticGenerationLgbParamAlloc();
//   data->init_fun = lgb_init;
//   data->ocb = ocb;
//   data->proofstate = proofstate;
//   data->model1 = model1;
//   //data->model2 = model2;
//
//   return WFCBAlloc(
//      EnigmaticGenerationLgbCompute,
//      prio_fun,
//      EnigmaticGenerationLgbExit,
//      data);
//}

double EnigmaticGenerationPredictLgb(Clause_p clause, EnigmaticGenerationLgbParam_p local, EnigmaticModel_p model)
{
   if (!model)
   {
      model = local->model1;
   }
   local->lgb_count = 0;
   return EnigmaticPredict(clause, model, local, local->fill_fun, local->predict_fun);
}

double EnigmaticGenerationPredictSetLgb(ClauseSet_p parents, EnigmaticGenerationLgbParam_p local, EnigmaticModel_p model)
{
   if (!model)
   {
      model = local->model1;
   }
   local->lgb_count = 0;
   return EnigmaticPredictSet(parents, model, local, local->fill_fun, local->predict_fun);
}

double EnigmaticGenerationPredictParentsLgb(Clause_p parent1, Clause_p parent2, EnigmaticGenerationLgbParam_p local, EnigmaticModel_p model)
{
   if (!model)
   {
      model = local->model1;
   }
   local->lgb_count = 0;
   if (parent2)
   {
	   return EnigmaticPredictParents(parent1, parent2, model, local, local->fill_fun, local->predict_fun);
   }
   return EnigmaticPredict(parent1, model, local, local->fill_fun, local->predict_fun);
}

double EnigmaticGenerationLgbCompute(void* data, Clause_p clause)
{
   EnigmaticGenerationLgbParam_p local = data;
   double pred, res;

   local->init_fun(data);
   pred = EnigmaticGenerationPredictLgb(clause, local, local->model1);
   // This function takes the prediction and converts it to a weight, which, frankly, can be kept for now.
   // We will still need to filter based on this.
   res = EnigmaticWeight(pred, local->model1->weight_type, local->model1->threshold);
   //if (local->model2)
   //{
   //   if (res == EW_POS)
   //   {
   //      pred = EnigmaticGenerationPredictLgb(clause, local, local->model2);
   //      res = EnigmaticGeneration(pred, local->model2->weight_type, local->model2->threshold);
   //   }
   //   else
   //   {
   //      res = EW_WORST;
   //   }
   //}

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

bool EnigmaticLgbFilterGenerationCompute(EnigmaticGenerationLgbParam_p local, Clause_p clause)
{
	//EnigmaticGenerationLgbParam_p local = control->enigma_gen_model;
	double pred;
	bool res = false;

	//Clause_p clause;
	PStackPointer j, sp;
	DerivationCode op;
	Clause_p parent1, parent2;
	//ClauseSet_p parents = ClauseSetAlloc();

	long resp = 0;

	local->init_fun(local); // If the model is initialized, this is just a quick if-statement

	// We want to get the parents.  So I'll try printing them first.

	sp = PStackGetSP(clause->derivation);
	j = 0;
	//resp = 0;

	//if (parent1)
	//{
	//	fprintf(GlobalOut, " #parent%ld ", resp);
	//}
	// I think we can rely on clauses having at most 2 parents by this point, before forward contraction steps that would add unit clauses and the like!
	// Equality factoring and resolution would have one 'parent', i.e., the more complicated expression for the same clause before unification.
	// Paramodulation adds two parents:
	// ClausePushDerivation(clause,  pm_type==ParamodPlain?DCParamod:DCSimParamod, pminfo->into, pminfo->new_orig);
	// The ClauseSet doesn't work as clauses are already in sets.  Copy them or add to a PStack but if they're always too, just passing them as parameters is most efficient!
	// The generic framework can be made later if E'd like to have the addons.
	//while (j < sp)
	//{
	op = PStackElementInt(clause->derivation, j);
	j++;

	if(DCOpHasCnfArg1(op))
	{
	   parent1 = PStackElementP(clause->derivation, j);
	   j++; //resp++;
	   if (OutputLevel>=1)
	   	{
		   fprintf(GlobalOut, " #parent1");//%ld ", resp);
		   ClausePrint(GlobalOut, parent1, true);
		   fprintf(GlobalOut, " ");
	   	}
	   //ClauseSetInsert(parents, parent);

	}
	if(DCOpHasCnfArg2(op))
	{
	   parent2 = PStackElementP(clause->derivation, j);
	   j++; //resp++;
	   if (OutputLevel>=1)
	   	{
		   fprintf(GlobalOut, " #parent2");//%ld ", resp);
		   ClausePrint(GlobalOut, parent2, true);
		   fprintf(GlobalOut, " ");
		   fprintf(GlobalOut, "\n");
	   	}
	   //ClauseSetInsert(parents, parent);
	}
	//}
	//if (sp == 0)
	//{
	//	fprintf(GlobalOut, " #sp == 0 ");
	//}

	//ClauseSetInsert(parents, clause);
	//pred = EnigmaticGenerationPredictLgb(clause, local, local->model1);
	//res = EnigmaticWeight(pred, local->model1->weight_type, local->model1->threshold);
	if (parent1) // || parent2)
	{
		pred = EnigmaticGenerationPredictParentsLgb(parent1, parent2, local, local->model1);
		//res = (pred <= local->model1->threshold);
		res = (pred <= 0.1);
	}
	if (OutputLevel>=1)
	{
	  if (OutputLevel>=2)
	  {
		 fprintf(GlobalOut, "+? ");
		 PrintEnigmaticVector(GlobalOut, local->model1->vector);
		 fprintf(GlobalOut, "\n");
	  }
	  //fprintf(GlobalOut, "=%.2f (val=%.3f,vlen=%d) : ", res, pred, local->lgb_count);
	  fprintf(GlobalOut, "=%s (val=%.3f,vlen=%d) : ", res ? "true" : "false", pred, local->lgb_count);
	  ClausePrint(GlobalOut, clause, true);
	  fprintf(GlobalOut, "\n");
	}

	//ClauseSetFree(parents);
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

