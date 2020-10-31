/*-----------------------------------------------------------------------

File  : che_enigmaticweightlgb.h

Author: Stephan Schultz, AI4REASON

Contents
 
  Copyright 2020 by the authors.
  This code is released under the GNU General Public Licence.
  See the file COPYING in the main CLIB directory for details.
  Run "eprover -h" for contact information.

Changes

<1> Fri 10 Apr 2020 11:14:30 PM CEST

-----------------------------------------------------------------------*/

#ifndef CHE_ENIGMATICWEIGHTLGB

#define CHE_ENIGMATICWEIGHTLGB

#include <ccl_relevance.h>
#include <che_refinedweight.h>
#include <che_enigmaticvectors.h>
#include "lightgbm.h"


/*---------------------------------------------------------------------*/
/*                    Data type declarations                           */
/*---------------------------------------------------------------------*/

typedef struct enigmaticweightlgbparamcell
{
   OCB_p        ocb;
   ProofState_p proofstate;

   char* model_filename;
   char* features_filename;
   int binary_weights;
   double threshold;

   BoosterHandle lgb_model;
   EnigmaticVector_p vector;
   EnigmaticInfo_p info;

   int32_t* lgb_indices;
   float* lgb_data;
   int lgb_count;
   
   void   (*init_fun)(struct enigmaticweightlgbparamcell*);
}EnigmaticWeightLgbParamCell, *EnigmaticWeightLgbParam_p;

/*---------------------------------------------------------------------*/
/*                Exported Functions and Variables                     */
/*---------------------------------------------------------------------*/

#define EnigmaticWeightLgbParamCellAlloc() (EnigmaticWeightLgbParamCell*) \
        SizeMalloc(sizeof(EnigmaticWeightLgbParamCell))
#define EnigmaticWeightLgbParamCellFree(junk) \
        SizeFree(junk, sizeof(EnigmaticWeightLgbParamCell))

EnigmaticWeightLgbParam_p EnigmaticWeightLgbParamAlloc(void);
void EnigmaticWeightLgbParamFree(EnigmaticWeightLgbParam_p junk);

WFCB_p EnigmaticWeightLgbParse(
   Scanner_p in, 
   OCB_p ocb, 
   ProofState_p state);

WFCB_p EnigmaticWeightLgbInit(
   ClausePrioFun prio_fun, 
   OCB_p ocb,
   ProofState_p proofstate,
   char* model_filename,
   char* features_filename,
   int binary_weights,
   double threshold);

double EnigmaticWeightLgbCompute(void* data, Clause_p clause);

void EnigmaticWeightLgbExit(void* data);

#endif

/*---------------------------------------------------------------------*/
/*                        End of File                                  */
/*---------------------------------------------------------------------*/

