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

/*---------------------------------------------------------------------*/
/*                    Data type declarations                           */
/*---------------------------------------------------------------------*/

/*---------------------------------------------------------------------*/
/*                Exported Functions and Variables                     */
/*---------------------------------------------------------------------*/

void EnigmaticClause(EnigmaticClause_p enigma, Clause_p clause, EnigmaticInfo_p info);

void EnigmaticClauseSet(EnigmaticClause_p enigma, ClauseSet_p set, EnigmaticInfo_p info);

void EnigmaticTheory(EnigmaticVector_p vector, ClauseSet_p axioms, EnigmaticInfo_p info);

void EnigmaticGoal(EnigmaticVector_p vector, ClauseSet_p goal, EnigmaticInfo_p info);

void EnigmaticProblem(EnigmaticVector_p vector, ClauseSet_p problem, EnigmaticInfo_p info);

#endif

/*---------------------------------------------------------------------*/
/*                        End of File                                  */
/*---------------------------------------------------------------------*/

