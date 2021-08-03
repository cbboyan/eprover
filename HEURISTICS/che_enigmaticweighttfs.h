/*-----------------------------------------------------------------------

File  : che_enigmaticweighttf.h

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

#ifndef CHE_ENIGMATICWEIGHTTF

#define CHE_ENIGMATICWEIGHTTF

#include <sys/socket.h>
#include <arpa/inet.h>
#include <ccl_relevance.h>
#include <che_refinedweight.h>
#include <che_enigmatictensors.h>
#include <che_enigmaticweightlgb.h>

/*---------------------------------------------------------------------*/
/*                    Data type declarations                           */
/*---------------------------------------------------------------------*/

typedef struct enigmaticweighttfparamcell
{
   OCB_p        ocb;
   ProofState_p proofstate;
   //TB_p         tmp_bank;
   //long         tmp_bank_vars;

   char* server_ip;
   uint16_t server_port;
   long context_size;
   double context_fixed_ratio;
   long context_size_fixed;
   long context_size_variable;
   int weight_type;
   double threshold;

   // fluent context handling
   PStack_p conj_clauses;
   PStack_p ctx_fixed;
   FloatTree_p ctx_variable;
   long ctx_var_cnt;
   long ctx_var_total;
   double ctx_var_worst;

   bool inited;

   void (*init_fun)(struct enigmaticweighttfparamcell*);

   EnigmaticTensors_p tensors;
   EnigmaticSocket_p sock; // socket data
   EnigmaticWeightLgbParam_p lgb;

}EnigmaticWeightTfsParamCell, *EnigmaticWeightTfsParam_p;

/*---------------------------------------------------------------------*/
/*                Exported Functions and Variables                     */
/*---------------------------------------------------------------------*/

#define EnigmaticWeightTfsParamCellAlloc() (EnigmaticWeightTfsParamCell*) \
        SizeMalloc(sizeof(EnigmaticWeightTfsParamCell))
#define EnigmaticWeightTfsParamCellFree(junk) \
        SizeFree(junk, sizeof(EnigmaticWeightTfsParamCell))

EnigmaticWeightTfsParam_p EnigmaticWeightTfsParamAlloc(void);
void EnigmaticWeightTfsParamFree(EnigmaticWeightTfsParam_p junk);

WFCB_p EnigmaticWeightTfsParse(
   Scanner_p in, 
   OCB_p ocb, 
   ProofState_p state);

WFCB_p EnigmaticWeightTfsInit(
   ClausePrioFun prio_fun, 
   OCB_p ocb,
   ProofState_p proofstate,
   char* server_ip,
   int server_port,
   long context_size,
   double context_fixed_ratio,
   int weight_type,
   double threshold,
   EnigmaticWeightLgbParam_p lgb);

double EnigmaticWeightTfsCompute(void* data, Clause_p clause);

void EnigmaticWeightTfsExit(void* data);

void EnigmaticWeightTfsEvalAxioms(ClauseSet_p axioms);

//void EnigmaticComputeEvals(ClauseSet_p set, EnigmaticWeightTfsParam_p local);

//void EnigmaticComputeEvalsSimple(ClauseSet_p set, EnigmaticWeightTfsParam_p local);

//void EnigmaticContextAdd(Clause_p clause, EnigmaticWeightTfsParam_p local);

#endif

/*---------------------------------------------------------------------*/
/*                        End of File                                  */
/*---------------------------------------------------------------------*/

