/*-----------------------------------------------------------------------

File  : che_enigmaweightemb.h

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

#ifndef CHE_ENIGMAWEIGHTEMB

#define CHE_ENIGMAWEIGHTEMB

#include <ccl_relevance.h>
#include <che_refinedweight.h>
#include <che_enigma.h>
#include "xgboost.h"


/*---------------------------------------------------------------------*/
/*                    Data type declarations                           */
/*---------------------------------------------------------------------*/

//#define EMB_LEN 448
#define EMB_LEN 64
#define STATS_LEN 25
#define STATS_COUNT_OFFSET 13

typedef struct enigmaweightembparamcell
{
   OCB_p        ocb;
   ProofState_p proofstate;
   char* model_filename;
   bool inited;
   NumTree_p embeds;
   BoosterHandle xgboost_model;
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
   void   (*init_fun)(struct enigmaweightembparamcell*);
}EnigmaWeightEmbParamCell, *EnigmaWeightEmbParam_p;

/*---------------------------------------------------------------------*/
/*                Exported Functions and Variables                     */
/*---------------------------------------------------------------------*/

extern char* EmbFileName;

#define EnigmaWeightEmbParamCellAlloc() (EnigmaWeightEmbParamCell*) \
        SizeMalloc(sizeof(EnigmaWeightEmbParamCell))
#define EnigmaWeightEmbParamCellFree(junk) \
        SizeFree(junk, sizeof(EnigmaWeightEmbParamCell))

EnigmaWeightEmbParam_p EnigmaWeightEmbParamAlloc(void);
void              EnigmaWeightEmbParamFree(EnigmaWeightEmbParam_p junk);


WFCB_p EnigmaWeightEmbParse(
   Scanner_p in, 
   OCB_p ocb, 
   ProofState_p state);

WFCB_p EnigmaWeightEmbInit(
   ClausePrioFun prio_fun, 
   OCB_p ocb,
   ProofState_p proofstate,
   char* model_filename);

double EnigmaWeightEmbCompute(void* data, Clause_p clause);

void EnigmaWeightEmbExit(void* data);

#endif

/*---------------------------------------------------------------------*/
/*                        End of File                                  */
/*---------------------------------------------------------------------*/

