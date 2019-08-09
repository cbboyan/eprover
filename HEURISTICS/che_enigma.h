/*-----------------------------------------------------------------------

File  : che_enigma.c

Author: Jan Jakubuv

Contents
 
  Copyright 2016 by the author.
  This code is released under the GNU General Public Licence and
  the GNU Lesser General Public License.
  See the file COPYING in the main E directory for details..
  Run "eprover -h" for contact information.

Changes

<1> Sat Jul  5 02:28:25 MET DST 1997
    New

-----------------------------------------------------------------------*/

#ifndef CHE_ENIGMA

#define CHE_ENIGMA

#include <clb_fixdarrays.h>
#include <clb_numtrees.h>
#include <ccl_clauses.h>
#include <che_clausesetfeatures.h>

#include "linear.h"
#include "svdlib.h"

/*---------------------------------------------------------------------*/
/*                    Data type declarations                           */
/*---------------------------------------------------------------------*/

typedef enum
{
   EFNone = 0,
   EFVertical = 1,
   EFHorizontal = 2,
   EFSymbols = 4,
   EFLengths = 8,
   EFConjecture = 16,
   EFProofWatch = 32,
   EFVariables = 64,
   EFHashing = 128,
   EFArity = 256,
   EFProblem = 512,
   EFSine = 1024,
   EFAll = 0xFFFF
}EnigmaFeature;

typedef long EnigmaFeatures;

typedef struct enigmapcell
{
   Sig_p sig;
   EnigmaFeatures version;
   
   StrTree_p feature_map;
   long feature_count;

   bool collect_stats;
   StrTree_p stats;
   StrTree_p name_cache;

   float problem_features[22];
   long* symb_rank;
   long symb_count;

} EnigmapCell, *Enigmap_p;

#define EnigmapCellAlloc() (EnigmapCell*) \
        SizeMalloc(sizeof(EnigmapCell))
#define EnigmapCellFree(junk) \
        SizeFree(junk, sizeof(EnigmapCell))

/*---------------------------------------------------------------------*/
/*                Exported Functions and Variables                     */
/*---------------------------------------------------------------------*/

#define ENIGMA_POS "+"
#define ENIGMA_NEG "-"
#define ENIGMA_VAR "*"
#define ENIGMA_SKO "?"
#define ENIGMA_EQ "="

Enigmap_p EnigmapAlloc(void);
void EnigmapFree(Enigmap_p junk);

Enigmap_p EnigmapLoad(char* features_filename, Sig_p sig);
void EnigmapFillProblemFeatures(Enigmap_p enigmap, ClauseSet_p axioms);
void EnigmapMarkConjectureSymbols(Enigmap_p enigmap, Clause_p clause);

EnigmaFeatures ParseEnigmaFeaturesSpec(char *spec);

DStr_p FeaturesGetTermHorizontal(char* top, Term_p term, Enigmap_p enigmap);
DStr_p FeaturesGetTermVertical(char* sym1, char* sym2, char* sym3);
DStr_p FeaturesGetEqHorizontal(Term_p lterm, Term_p rterm, Enigmap_p enigmap);

void FeaturesClauseVariablesExtend(Enigmap_p enigmap, NumTree_p* stat, Clause_p clause, int* distinct, int offset);
void FeaturesClauseVariablesStat(NumTree_p* stat, long* out, bool pos_keys);
void FeaturesAddVariables(NumTree_p* counts, NumTree_p* varstat, Enigmap_p enigmap, int *len);

int FeaturesClauseExtend(NumTree_p* counts, Clause_p clause, Enigmap_p enigmap);
void FeaturesAddClauseStatic(NumTree_p* counts, Clause_p clause, Enigmap_p enigmap, int *len,
         NumTree_p* varstat, int* varoffset);
NumTree_p FeaturesClauseCollect(Clause_p clause, Enigmap_p enigmap, int* len);

void FeaturesSvdTranslate(DMat matUt, double* sing, 
   struct feature_node* in, struct feature_node* out);

#endif

/*---------------------------------------------------------------------*/
/*                        End of File                                  */
/*---------------------------------------------------------------------*/

