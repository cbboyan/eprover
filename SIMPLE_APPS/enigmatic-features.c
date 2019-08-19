/*-----------------------------------------------------------------------

File  : enigmatic-features.c

Author: Stephan Schultz, Josef Urban, Jan Jakubuv

Contents
 
  Copyright 2019 by the authors.
  This code is released under the GNU General Public Licence.
  See the file COPYING in the main CLIB directory for details.
  Run "eprover -h" for contact information.

Changes

<1> Sat 17 Aug 2019 05:10:23 PM CEST

-----------------------------------------------------------------------*/

#include <stdio.h>
#include <string.h>
#include <cio_commandline.h>
#include <cio_output.h>
#include <ccl_proofstate.h>

/*---------------------------------------------------------------------*/
/*                  Data types                                         */
/*---------------------------------------------------------------------*/

typedef enum
{
   OPT_NOOPT=0,
   OPT_HELP,
   OPT_VERBOSE,
   OPT_OUTPUT,
   OPT_FREE_NUMBERS,
}OptionCodes;

typedef struct enigmaticcell
{
   Sig_p sig;
} EnigmaticCell, *Enigmatic_p;

#define EnigmaticCellAlloc() (EnigmaticCell*) \
        SizeMalloc(sizeof(EnigmaticCell))
#define EnigmaticCellFree(junk) \
        SizeFree(junk, sizeof(EnigmaticCell))

typedef struct enigmaticinfocell
{
   long len;
   long pos;
   long neg;
   long max_depth;
   long avg_depth;
   long width;
   long avg_depth;
   long vars;
   long pos_eqs;
   long neg_eqs;

   // f(X,X,Y,Y)

   long var_hist[6];
   long func_hist[6];
   long pred_hist[6];

   long var_count[6];
   long func_count[6];
   long pred_count[6];

   long var_rat[6];
   long func_rat[6];
   long pred_rat[6];

} EnigmaticInfoCell, *EnigmaticInfo_p;

#define EnigmaticInfoCellAlloc() (EnigmaticInfoCell*) \
        SizeMalloc(sizeof(EnigmaticInfoCell))
#define EnigmaticInfoCellFree(junk) \
        SizeFree(junk, sizeof(EnigmaticInfoCell))

/*---------------------------------------------------------------------*/
/*                        Global Variables                             */
/*---------------------------------------------------------------------*/

OptCell opts[] =
{
   {OPT_HELP, 
      'h', "help", 
      NoArg, NULL,
      "Print a short description of program usage and options."},
   {OPT_VERBOSE, 
      'v', "verbose", 
      OptArg, "1",
      "Verbose comments on the progress of the program."},
   {OPT_OUTPUT,
      'o', "output-file",
      ReqArg, NULL,
      "Redirect output into the named file."},
   {OPT_FREE_NUMBERS,
      '\0', "free-numbers",
      NoArg, NULL,
      "Treat numbers (strings of decimal digits) as normal free function "
         "symbols in the input. By default, number now are supposed to denote"
         " domain constants and to be implicitly different from each other."},
   {OPT_NOOPT,
      '\0', NULL,
      NoArg, NULL,
      NULL}
};

char *outname = NULL;
FunctionProperties free_symb_prop = FPIgnoreProps;

/*---------------------------------------------------------------------*/
/*                      Forward Declarations                           */
/*---------------------------------------------------------------------*/

CLState_p process_options(int argc, char* argv[]);
void print_help(FILE* out);

/*---------------------------------------------------------------------*/
/*                         Internal Functions                          */
/*---------------------------------------------------------------------*/

Enigmatic_p EnigmaticAlloc(void)
{
   Enigmatic_p res = EnigmaticCellAlloc();

   res->sig = NULL;

   return res;
}

void EnigmaticFree(Enigmatic_p junk)
{
   EnigmaticCellFree(junk);
}

EnigmaticInfo_p EnigmaticInfoAlloc(void)
{
   int i;
   EnigmaticInfo_p res = EnigmaticInfoCellAlloc();

   res->len = 0L;
   res->pos = 0L; 
   res->neg = 0L;
   res->depth = 0L;
   res->width = 0L;
   res->vars = 0L;
   res->eqs = 0L;

   for (i=0; i<6; i++) { res->var_hist[i] = 0L; }
   for (i=0; i<6; i++) { res->func_hist[i] = 0L; }
   for (i=0; i<6; i++) { res->pred_hist[i] = 0L; }

   for (i=0; i<6; i++) { res->var_count[i] = 0L; }
   for (i=0; i<6; i++) { res->func_count[i] = 0L; }
   for (i=0; i<6; i++) { res->pred_count[i] = 0L; }

   for (i=0; i<6; i++) { res->var_rat[i] = 0L; }
   for (i=0; i<6; i++) { res->func_rat[i] = 0L; }
   for (i=0; i<6; i++) { res->pred_rat[i] = 0L; }

   return res;
}

void EnigmaticInfoFree(EnigmaticInfo_p junk)
{
   EnigmaticInfoCellFree(junk);
}

static void info_term(EnigmaticInfo_p info, Term_p term, int depth, bool pos)
{  
   if (TermIsVar(term))
   {
      info->vars++;
      info->depth = MAX(info->depth, depth);
      return;

   }

   //  P(a) | ~Q(a,a)
   // ~P(a) |  Q(a,a)
}

static void info_clause(EnigmaticInfo_p info, Clause_p clause)
{
   for (Eqn_p lit=clause->literals; lit; lit=lit->next)
   {
      bool pos = EqnIsPositive(lit);
      if (pos) { info->pos++; } else { info->neg++; }

      info_term(info, lit->lterm, 0, pos);
      if (lit->rterm->f_code != SIG_TRUE_CODE)
      {
         info->len++;
         info->eqs++;
         info_term(info, lit->rterm, 0, pos);
      }
   }

}

void EnigmaticInfoCellExtend(EnigmaticInfo_p info, Clause_p clause)
{
}

int main(int argc, char* argv[])
{
   InitIO(argv[0]);
   CLState_p args = process_options(argc, argv);
   //SetMemoryLimit(2L*1024*MEGA);
   OutputFormat = TSTPFormat;
   if (outname) { OpenGlobalOut(outname); }
   ProofState_p state = ProofStateAlloc(free_symb_prop);
  
   ProofStateFree(state);
   CLStateFree(args);
   ExitIO();

   return 0;
}

/*-----------------------------------------------------------------------
//
// Function: process_options()
//
//   Read and process the command line option, return (the pointer to)
//   a CLState object containing the remaining arguments.
//
// Global Variables: opts, Verbose, TermPrologArgs,
//                   TBPrintInternalInfo 
//
// Side Effects    : Sets variables, may terminate with program
//                   description if option -h or --help was present
//
/----------------------------------------------------------------------*/

CLState_p process_options(int argc, char* argv[])
{
   Opt_p handle;
   CLState_p state;
   char*  arg;
   
   state = CLStateAlloc(argc,argv);
   
   while((handle = CLStateGetOpt(state, &arg, opts)))
   {
      switch(handle->option_code)
      {
      case OPT_VERBOSE:
         Verbose = CLStateGetIntArg(handle, arg);
         break;
      case OPT_HELP: 
         print_help(stdout);
         exit(NO_ERROR);
      case OPT_OUTPUT:
         outname = arg;
         break;
      case OPT_FREE_NUMBERS:
         free_symb_prop = free_symb_prop|FPIsInteger|FPIsRational|FPIsFloat;
         break;
      default:
         assert(false);
         break;
      }
   }
   
   if (state->argc != 1)
   {
      print_help(stdout);
      exit(NO_ERROR);
   }
   
   return state;
}
 
void print_help(FILE* out)
{
   fprintf(out, "\n\
Usage: enigmatic-features [options] cnfs.tptp\n\
\n\
Make ENIGMA features from TPTP cnf clauses.\n\
\n");
   PrintOptions(stdout, opts, NULL);
}

/*---------------------------------------------------------------------*/
/*                        End of File                                  */
/*---------------------------------------------------------------------*/

