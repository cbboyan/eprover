/*-----------------------------------------------------------------------

File  : che_enigmaweightemblgb.h

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

#ifndef CHE_ENIGMAWEIGHTEMBLGB

#define CHE_ENIGMAWEIGHTEMBLGB

#include <ccl_relevance.h>
#include <che_refinedweight.h>
#include <che_enigma.h>
#include "lightgbm.h"


/*---------------------------------------------------------------------*/
/*                    Data type declarations                           */
/*---------------------------------------------------------------------*/

typedef struct enigmaweightemblgbparamcell
{
   OCB_p        ocb;
   ProofState_p proofstate;
   char* model_filename;
   bool inited;
   NumTree_p embeds;
   BoosterHandle lgb_model;
   double conj_emb[EMB_LEN];
   int conj_len;
   int conj_vars;
   int conj_stats[STATS_LEN];
   SpecFeature_p prob_spec;
   /*
   char* features_filename;

   double len_mult;

   BoosterHandle emboost_model;
   Enigmap_p enigmap;
   
   unsigned* conj_features_indices;
   float* conj_features_data;
   int conj_features_count;

   */
   void   (*init_fun)(struct enigmaweightemblgbparamcell*);
}EnigmaWeightEmbLgbParamCell, *EnigmaWeightEmbLgbParam_p;

/*---------------------------------------------------------------------*/
/*                Exported Functions and Variables                     */
/*---------------------------------------------------------------------*/

#define EnigmaWeightEmbLgbParamCellAlloc() (EnigmaWeightEmbLgbParamCell*) \
        SizeMalloc(sizeof(EnigmaWeightEmbLgbParamCell))
#define EnigmaWeightEmbLgbParamCellFree(junk) \
        SizeFree(junk, sizeof(EnigmaWeightEmbLgbParamCell))

EnigmaWeightEmbLgbParam_p EnigmaWeightEmbLgbParamAlloc(void);
void              EnigmaWeightEmbLgbParamFree(EnigmaWeightEmbLgbParam_p junk);


WFCB_p EnigmaWeightEmbLgbParse(
   Scanner_p in, 
   OCB_p ocb, 
   ProofState_p state);

WFCB_p EnigmaWeightEmbLgbInit(
   ClausePrioFun prio_fun, 
   OCB_p ocb,
   ProofState_p proofstate,
   char* model_filename);

double EnigmaWeightEmbLgbCompute(void* data, Clause_p clause);

void EnigmaWeightEmbLgbExit(void* data);

#endif

/*---------------------------------------------------------------------*/
/*                        End of File                                  */
/*---------------------------------------------------------------------*/

