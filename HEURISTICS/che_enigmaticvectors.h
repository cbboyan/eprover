/*-----------------------------------------------------------------------

File  : che_enigmaticvectors.h

Author: Stephan Schultz, AI4REASON

Contents
 
  Copyright 2020 by the authors.
  This code is released under the GNU General Public Licence.
  See the file COPYING in the main CLIB directory for details.
  Run "eprover -h" for contact information.

Changes

<1> Fri 10 Apr 2020 11:14:30 PM CEST

-----------------------------------------------------------------------*/

#ifndef CHE_ENIGMATICVECTORS

#define CHE_ENIGMATICVECTORS

#include <che_enigmaticdata.h>
#include <ccl_clausesets.h>
#include <ccl_formulasets.h>
#include <ccl_proofstate.h>

/*---------------------------------------------------------------------*/
/*                    Data type declarations                           */
/*---------------------------------------------------------------------*/

/*---------------------------------------------------------------------*/
/*                Exported Functions and Variables                     */
/*---------------------------------------------------------------------*/

void EnigmaticClause(EnigmaticClause_p enigma, Clause_p clause, EnigmaticInfo_p info);

void EnigmaticClause(EnigmaticClause_p enigma, Clause_p clause, EnigmaticInfo_p info);

Clause_p EnigmaticFormulaToClause(WFormula_p formula, EnigmaticInfo_p info);

void EnigmaticClauseSet(EnigmaticClause_p enigma, ClauseSet_p set, EnigmaticInfo_p info);

void EnigmaticTheory(EnigmaticVector_p vector, ClauseSet_p axioms, EnigmaticInfo_p info);

void EnigmaticGoal(EnigmaticVector_p vector, ClauseSet_p goal, EnigmaticInfo_p info);

void EnigmaticProblem(EnigmaticVector_p vector, ClauseSet_p problem, EnigmaticInfo_p info);

void EnigmaticInitProblem(EnigmaticVector_p vector, EnigmaticInfo_p info, 
      FormulaSet_p f_axioms, ClauseSet_p axioms);

void EnigmaticInitEval(char* features_filename, EnigmaticInfo_p* info, 
      EnigmaticVector_p* vector, ProofState_p proofstate);

void EnigmaticInit(EnigmaticModel_p model, ProofState_p proofstate);

double EnigmaticPredict(Clause_p clause, EnigmaticModel_p model, void* data,
   FillFunc fill_func, PredictFunc predict_func);

double EnigmaticWeight(double pred, int weight_type, double threshold);

#endif

/*---------------------------------------------------------------------*/
/*                        End of File                                  */
/*---------------------------------------------------------------------*/

