/*-----------------------------------------------------------------------

File  : che_enigmaticvectors.c

Author: Stephan Schultz, AI4REASON

Contents
 
  Copyright 2020 by the authors.
  This code is released under the GNU General Public Licence.
  See the file COPYING in the main CLIB directory for details.
  Run "eprover -h" for contact information.

Changes

<1> Fri 10 Apr 2020 11:14:30 PM CEST

-----------------------------------------------------------------------*/


#include "che_enigmaticvectors.h"



/*---------------------------------------------------------------------*/
/*                        Global Variables                             */
/*---------------------------------------------------------------------*/

/*---------------------------------------------------------------------*/
/*                      Forward Declarations                           */
/*---------------------------------------------------------------------*/

/*---------------------------------------------------------------------*/
/*                         Internal Functions                          */
/*---------------------------------------------------------------------*/

static void update_term(EnigmaticClause_p enigma, Term_p term, long depth)
{
   if (TermIsVar(term))
   {
      enigma->width++;
      enigma->depth = MAX(enigma->depth, depth+1);
   }
   else if (TermIsConst(term))
   {
      enigma->width++;
      enigma->depth = MAX(enigma->depth, depth+1);
   }
   else
   {
      for (int i=0; i<term->arity; i++)
      {
         update_term(enigma, term->args[i], depth+1);
      }
   }
   enigma->len++;
}

static void update_lit(EnigmaticClause_p enigma, Eqn_p lit)
{
   bool pos = EqnIsPositive(lit);
   if (lit->rterm->f_code == SIG_TRUE_CODE)
   {
      update_term(enigma, lit->lterm, pos ? 0 : 1);
      if (pos) { enigma->pos_atoms++; } else { enigma->neg_atoms++; }
   }
   else
   {
      enigma->len++; // count equality
      update_term(enigma, lit->lterm, 1);
      update_term(enigma, lit->rterm, 1);
      if (pos) { enigma->pos_eqs++; } else { enigma->neg_eqs++; }
   }
   if (pos) { enigma->pos++; } else { enigma->neg++; enigma->len++; }
   enigma->lits++;
}

#define PRINT_INT(key,val) if (val) fprintf(out, "%ld:%ld ", key, val)
#define PRINT_FLOAT(key,val) if (val) fprintf(out, "%ld:%.2f ", key, val)

static void dump_clause_block(FILE* out, EnigmaticClause_p clause)
{
   if (clause->params->use_len)
   {
      PRINT_INT(clause->params->offset_len+0,  clause->len);
      PRINT_INT(clause->params->offset_len+1,  clause->lits);
      PRINT_INT(clause->params->offset_len+2,  clause->pos);
      PRINT_INT(clause->params->offset_len+3,  clause->neg);
      PRINT_INT(clause->params->offset_len+4,  clause->depth);
      PRINT_INT(clause->params->offset_len+5,  clause->width);
      PRINT_FLOAT(clause->params->offset_len+6,  clause->avg_depth);
      PRINT_INT(clause->params->offset_len+7,  clause->pos_eqs);
      PRINT_INT(clause->params->offset_len+8,  clause->neg_eqs);
      PRINT_INT(clause->params->offset_len+9,  clause->pos_atoms);
      PRINT_INT(clause->params->offset_len+10, clause->neg_atoms);
   }
}

/*---------------------------------------------------------------------*/
/*                         Exported Functions                          */
/*---------------------------------------------------------------------*/

void EnigmaticClauseUpdate(EnigmaticClause_p enigma, Clause_p clause)
{
   long max_depth = enigma->depth;
   for (Eqn_p lit=clause->literals; lit; lit=lit->next)
   {
      enigma->depth = 0;
      update_lit(enigma, lit);
      enigma->avg_depth += enigma->depth; // temporalily the sum of literal depths
      max_depth = MAX(max_depth, enigma->depth);
   }
   enigma->depth = max_depth;
}

void PrintEnigmaticVector(FILE* out, EnigmaticVector_p vector)
{
   dump_clause_block(out, vector->clause);
}


/*---------------------------------------------------------------------*/
/*                        End of File                                  */
/*---------------------------------------------------------------------*/

