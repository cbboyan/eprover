/*-----------------------------------------------------------------------

File  : che_enigmaticweightxgb.h

Author: Stephan Schultz, AI4REASON

Contents
 
  Copyright 2020 by the authors.
  This code is released under the GNU General Public Licence.
  See the file COPYING in the main CLIB directory for details.
  Run "eprover -h" for contact information.

Changes

<1> Fri 10 Apr 2020 11:14:30 PM CEST

-----------------------------------------------------------------------*/

#ifndef CHE_ENIGMATICWEIGHTXGB

#define CHE_ENIGMATICWEIGHTXGB

#include <ccl_relevance.h>
#include <che_refinedweight.h>
#include <che_enigmaticvectors.h>
#include "xgboost.h"


/*---------------------------------------------------------------------*/
/*                    Data type declarations                           */
/*---------------------------------------------------------------------*/

typedef struct enigmaticweightxgbparamcell
{
   OCB_p        ocb;
   ProofState_p proofstate;

   EnigmaticModel_p model1;
   EnigmaticModel_p model2;

   unsigned int* xgb_indices;
   float* xgb_data;
   int xgb_count;
   long xgb_size;
   
   void   (*init_fun)(struct enigmaticweightxgbparamcell*);
} EnigmaticWeightXgbParamCell, *EnigmaticWeightXgbParam_p;

/*---------------------------------------------------------------------*/
/*                Exported Functions and Variables                     */
/*---------------------------------------------------------------------*/

#define EnigmaticWeightXgbParamCellAlloc() (EnigmaticWeightXgbParamCell*) \
        SizeMalloc(sizeof(EnigmaticWeightXgbParamCell))
#define EnigmaticWeightXgbParamCellFree(junk) \
        SizeFree(junk, sizeof(EnigmaticWeightXgbParamCell))

EnigmaticWeightXgbParam_p EnigmaticWeightXgbParamAlloc(void);
void EnigmaticWeightXgbParamFree(EnigmaticWeightXgbParam_p junk);

WFCB_p EnigmaticWeightXgbParse(
   Scanner_p in, 
   OCB_p ocb, 
   ProofState_p state);

WFCB_p EnigmaticWeightXgbInit(
   ClausePrioFun prio_fun, 
   OCB_p ocb,
   ProofState_p proofstate,
   EnigmaticModel_p model1,
   EnigmaticModel_p model2);

double EnigmaticWeightXgbCompute(void* data, Clause_p clause);

void EnigmaticWeightXgbExit(void* data);

#endif

/*---------------------------------------------------------------------*/
/*                        End of File                                  */
/*---------------------------------------------------------------------*/

