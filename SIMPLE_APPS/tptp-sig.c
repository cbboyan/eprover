/*-----------------------------------------------------------------------

File  : fofshared.c

Author: Josef Urban

Contents
 
  Read an initial set of fof terms and print the (shared) codes of all subterms
  present in them. If file names given (or file and stdin), read both
  in, but only print the codes for the second one (this is intended to
  allow consistent codes over several runs).
 

  Copyright 1998, 1999 by the author.
  This code is released under the GNU General Public Licence.
  See the file COPYING in the main CLIB directory for details.
  Run "eprover -h" for contact information.

Changes

<1> Fri Nov 28 00:27:40 MET 1997

-----------------------------------------------------------------------*/

#include <stdio.h>
#include <string.h>
#include <cio_commandline.h>
#include <cio_output.h>
#include <cte_termbanks.h>
#include <ccl_formulafunc.h>
#include <ccl_proofstate.h>

/*---------------------------------------------------------------------*/
/*                  Data types                                         */
/*---------------------------------------------------------------------*/

typedef enum
{
   OPT_NOOPT=0,
   OPT_HELP,
   OPT_VERBOSE,
   OPT_FREE_NUMBERS,
   OPT_OUTPUT
}OptionCodes;

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


int main(int argc, char* argv[])
{
   InitIO(argv[0]);
   CLState_p args = process_options(argc, argv);
   //SetMemoryLimit(2L*1024*MEGA);
   OutputFormat = TSTPFormat;
   if (outname) { OpenGlobalOut(outname); }
   ProofState_p state = ProofStateAlloc(free_symb_prop);

   // parse data
   StrTree_p skip_includes = NULL;
   Scanner_p in = CreateScanner(StreamTypeFile, args->argv[0], true, NULL);
   ScannerSetFormat(in, TSTPFormat);
   FormulaAndClauseSetParse(in, state->f_axioms, state->watchlist, 
      state->terms, NULL, &skip_includes);
   CheckInpTok(in, NoToken);
   DestroyScanner(in);

   //SigPrint(GlobalOut, state->signature);
   
   // dump predicates
   Sig_p sig = state->signature;
   bool first = true;
   int i;
   for (i=1; i<=sig->f_count; i++)
   {
      if (SigIsSpecial(sig, i) || !SigIsPredicate(sig, i)) 
      { 
         continue; 
      }
      fprintf(GlobalOut, "%s%s/%d", first ? "" : ";",
         SigFindName(sig, i), SigFindArity(sig, i));
      first = false;
   }
   fprintf(GlobalOut, "|");
  
   // dump function symbols

   first = true;
   for (i=1; i<=sig->f_count; i++)
   {
      if (SigIsSpecial(sig, i) || SigIsPredicate(sig, i)) 
      { 
         continue; 
      }
      fprintf(GlobalOut, "%s%s/%d", first ? "" : ";",
         SigFindName(sig, i), SigFindArity(sig, i));
      first = false;
   }
   fprintf(GlobalOut, "\n");
  
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

   if (state->argc != 1 && state->argc != 3)
   {
      print_help(stdout);
      exit(NO_ERROR);
   }
   
   return state;
}
 
void print_help(FILE* out)
{
   fprintf(out, "\n\
\n\
Usage: enigma-features [options] cnfs.tptp\n\
   or: enigma-features [options] train.pos train.neg [train.cnf]\n\
\n\
Make enigma features from TPTP cnf clauses.  Optionally prefixing with\n\
sign (+ or -), and postfixing with conjecture features from train.cnf.\n\
Output line format is 'sign|clause|conjecture'.\n\
\n");
   PrintOptions(stdout, opts, NULL);
}


/*---------------------------------------------------------------------*/
/*                        End of File                                  */
/*---------------------------------------------------------------------*/


