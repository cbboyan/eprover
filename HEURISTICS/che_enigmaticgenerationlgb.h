/*-----------------------------------------------------------------------

File  : che_enigmaticgenerationlgb.h

Author: Stephan Schultz, AI4REASON

Contents
 
  Copyright 2020 by the authors.
  This code is released under the GNU General Public Licence.
  See the file COPYING in the main CLIB directory for details.
  Run "eprover -h" for contact information.

Changes

<1> Fri 10 Apr 2020 11:14:30 PM CEST

-----------------------------------------------------------------------*/

#ifndef CHE_ENIGMATICGENERATIONLGB

#define CHE_ENIGMATICGENERATIONLGB

#include <ccl_relevance.h>
#include <che_refinedweight.h>
#include <che_enigmaticvectors.h>
#include "lightgbm.h"


/*---------------------------------------------------------------------*/
/*                    Data type declarations                           */
/*---------------------------------------------------------------------*/

typedef struct enigmaticgenerationlgbparamcell
{
   OCB_p        ocb;
   ProofState_p proofstate;

   EnigmaticModel_p model1;

   int32_t* lgb_indices;
   float* lgb_data;
   int lgb_count;
   long lgb_size;
   bool concat;

   FillFunc fill_fun;
   PredictFunc predict_fun;
   LoadFunc load_fun;
   void   (*init_fun)(struct enigmaticgenerationlgbparamcell*);
} EnigmaticGenerationLgbParamCell, *EnigmaticGenerationLgbParam_p;

/*---------------------------------------------------------------------*/
/*                Exported Functions and Variables                     */
/*---------------------------------------------------------------------*/

#define EnigmaticGenerationLgbParamCellAlloc() (EnigmaticGenerationLgbParamCell*) \
        SizeMalloc(sizeof(EnigmaticGenerationLgbParamCell))
#define EnigmaticGenerationLgbParamCellFree(junk) \
        SizeFree(junk, sizeof(EnigmaticGenerationLgbParamCell))

EnigmaticGenerationLgbParam_p EnigmaticGenerationLgbParamAlloc(void);
void EnigmaticGenerationLgbParamFree(EnigmaticGenerationLgbParam_p junk);

void EnigmaticGenerationLgbModelInit(
	char* d_prefix,
	char* model_name,
	double threshold,
	OCB_p ocb,
	ProofState_p state,
	EnigmaticGenerationLgbParam_p data);

double EnigmaticGenerationPredictLgb(Clause_p clause, EnigmaticGenerationLgbParam_p local, EnigmaticModel_p model);
double EnigmaticGenerationPredictParentsLgb(Clause_p parent1, Clause_p parent2, EnigmaticGenerationLgbParam_p local, EnigmaticModel_p model);
double EnigmaticGenerationPredictParentsConcatLgb(Clause_p parent1, Clause_p parent2, EnigmaticGenerationLgbParam_p local, EnigmaticModel_p model);


bool EnigmaticLgbFilterGenerationCompute(EnigmaticGenerationLgbParam_p local, Clause_p clause);

void EnigmaticGenerationLgbExit(void* data);

#endif

/*---------------------------------------------------------------------*/
/*                        End of File                                  */
/*---------------------------------------------------------------------*/

