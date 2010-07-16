/*-----------------------------------------------------------------------

File  : ccl_f_generality.h

Author: Stephan Schulz (schulz@eprover.org)

Contents
 
  Code for computing the generality of function/predicate symbols
  using a generalize SinE approach, counting occurences in terms,
  literals, clauses, and formulas.

  Copyright 2010 by the author.
  This code is released under the GNU General Public Licence.
  See the file COPYING in the main CLIB directory for details.
  Run "eprover -h" for contact information.

Changes

<1> Wed Jun 30 23:30:02 CEST 2010
    New

-----------------------------------------------------------------------*/

#ifndef CCL_F_GENERALITY

#define CCL_F_GENERALITY

#include <ccl_formulasets.h>
#include <ccl_clausesets.h>


/*---------------------------------------------------------------------*/
/*                    Data type declarations                           */
/*---------------------------------------------------------------------*/


/* Type of generality measure: Number of occurences in terms or in
 * formulas (and possibly later in equations). */

typedef enum
{
   GMTerms,
   GMFormulas
}GeneralityMeasure;


/* How often does a give f_code occur in certain substructures? */

typedef struct fun_gen_cell
{
   FunCode f_code;
   long    term_freq;
   long    fc_freq;   
}FunGenCell, *FunGen_p;

/* Distribution of the above... */

typedef struct gen_distrib_cell
{
   Sig_p    sig;
   long     size;
   FunGen_p dist_array;  
}GenDistribCell, *GenDistrib_p;



/*---------------------------------------------------------------------*/
/*                Exported Functions and Variables                     */
/*---------------------------------------------------------------------*/

#define GenDistribCellAlloc()    (GenDistribCell*)SizeMalloc(sizeof(GenDistribCell))
#define GenDistribCellFree(junk) SizeFree(junk, sizeof(GenDistribCell))

GenDistrib_p GenDistribAlloc(Sig_p sig);
void         GenDistribFree(GenDistrib_p junk);
void         GenDistribSizeAdjust(GenDistrib_p gd, Sig_p sig);

void         GenDistribAddClause(GenDistrib_p dist, Clause_p clause, 
                                 short factor);
void         GenDistribAddClauseSet(GenDistrib_p dist, ClauseSet_p set, 
                                    short factor);

void         GenDistribAddFormula(GenDistrib_p dist, WFormula_p form, 
                                  short factor);
void         GenDistribAddFormulaSet(GenDistrib_p dist, 
                                     FormulaSet_p set, 
                                     short factor);

void         GenDistribAddClauseSetStack(GenDistrib_p dist, 
                                         PStack_p stack,
                                         PStackPointer start,
                                         short factor);
void         GenDistribAddFormulaSetStack(GenDistrib_p dist, 
                                          PStack_p stack, 
                                          PStackPointer start,
                                          short factor);

#define GenDistribAddClauseSets(dist, stack) \
   GenDistribAddClauseSetStack((dist), (stack), 0, 1)

#define GenDistribAddFormulaSets(dist, stack) \
   GenDistribAddFormulaSetStack((dist), (stack), 0, 1)


#define GenDistribBacktrackClauseSets(dist, stack, sp)     \
   GenDistribAddClauseSetStack((dist), (stack), (sp), -1)

#define GenDistribBacktrackFormulaSets(dist, stack, sp)          \
   GenDistribAddFormulaSetStack((dist), (stack), (sp), -1)


void         GenDistPrint(FILE* out, GenDistrib_p dist);

int          FunGenTGCmp(const FunGen_p fg1, const FunGen_p fg2);
int          FunGenCGCmp(const FunGen_p fg1, const FunGen_p fg2);

void        ClauseComputeDRel(GenDistrib_p generality, 
                              GeneralityMeasure gentype,
                              double benevolence,
                              Clause_p clause, 
                              PStack_p res);
void        FormulaComputeDRel(GenDistrib_p generality, 
                               GeneralityMeasure gentype,
                               double benevolence,
                               WFormula_p form, 
                               PStack_p res);

#endif

/*---------------------------------------------------------------------*/
/*                        End of File                                  */
/*---------------------------------------------------------------------*/




