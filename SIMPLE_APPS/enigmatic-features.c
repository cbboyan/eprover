/*-----------------------------------------------------------------------

File  : enigmatic-features.c

Author: Stephan Schultz, AI4REASON

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
#include <che_enigmaticvectors.h>

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
   OPT_PROBLEM,
   OPT_FEATURES,
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
   {OPT_FEATURES,
      'f', "features",
      ReqArg, NULL,
      "Enigma features specifier string."},
   {OPT_PROBLEM,
      'p', "problem",
      ReqArg, NULL,
      "TPTP problem file in CNF for goal/theory features."},
   {OPT_NOOPT,
      '\0', NULL,
      NoArg, NULL,
      NULL}
};

char *outname = NULL;
FunctionProperties free_symb_prop = FPIgnoreProps;
EnigmaticFeatures_p features;
char* problem_file = NULL;

/*---------------------------------------------------------------------*/
/*                      Forward Declarations                           */
/*---------------------------------------------------------------------*/

CLState_p process_options(int argc, char* argv[]);
void print_help(FILE* out);

/*---------------------------------------------------------------------*/
/*                         Internal Functions                          */
/*---------------------------------------------------------------------*/

static void process_problem(char* problem_file, TB_p bank, EnigmaticVector_p vector, EnigmaticInfo_p info)
{
   if (!problem_file)
   {
      return;
   }

   ClauseSet_p theory = ClauseSetAlloc();
   ClauseSet_p goal = ClauseSetAlloc();
   
   Scanner_p in = CreateScanner(StreamTypeFile, problem_file, true, NULL);
   ScannerSetFormat(in, TSTPFormat);
   while (TestInpId(in, "cnf"))
   {
      Clause_p clause = ClauseParse(in, bank);
      if (ClauseQueryTPTPType(clause) == CPTypeNegConjecture)
      {
         ClauseSetInsert(goal, clause);
      }
      else
      {
         ClauseSetInsert(theory, clause);
      }
   }
   
   CheckInpTok(in, NoToken);
   DestroyScanner(in);

   EnigmaticGoal(vector, goal, info);
   EnigmaticTheory(vector, theory, info);

   ClauseSetFree(theory);
   ClauseSetFree(goal);
}

static void process_clauses(FILE* out, char* filename, TB_p bank, EnigmaticVector_p vector, EnigmaticInfo_p info)
{
   Scanner_p in = CreateScanner(StreamTypeFile, filename, true, NULL);
   ScannerSetFormat(in, TSTPFormat);
   
   while (TestInpId(in, "cnf"))
   {
      Clause_p clause = ClauseParse(in, bank);
      ClausePrint(out, clause, true);
      fprintf(out, "\n");

      EnigmaticClause(vector->clause, clause, info);
      PrintEnigmaticVector(GlobalOut, vector);
      fprintf(out, "\n");

      EnigmaticClauseReset(vector->clause);
      ClauseFree(clause);
   }

   CheckInpTok(in, NoToken);
   DestroyScanner(in);
}

int main(int argc, char* argv[])
{
   InitIO(argv[0]);
   CLState_p args = process_options(argc, argv);
   //SetMemoryLimit(2L*1024*MEGA);
   OutputFormat = TSTPFormat;
   if (outname) { OpenGlobalOut(outname); }
   ProofState_p state = ProofStateAlloc(free_symb_prop);
   EnigmaticVector_p vector = EnigmaticVectorAlloc(features);
   EnigmaticInfo_p info = EnigmaticInfoAlloc();
   info->sig = state->signature;
   info->collect_hashes = true;

   process_problem(problem_file, state->terms, vector, info);
   process_clauses(GlobalOut, args->argv[0], state->terms, vector, info);
   
   PrintEnigmaticFeaturesInfo(GlobalOut, features, "FIX-ME");
   PrintEnigmaticFeaturesMap(GlobalOut, features);
   PrintEnigmaticHashes(GlobalOut, info);
 
   EnigmaticVectorFree(vector);
   EnigmaticInfoFree(info);
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
      case OPT_FEATURES:
         features = EnigmaticFeaturesParse(arg);
         break;
      case OPT_PROBLEM:
         problem_file = arg;
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

   if (!features)
   {
      Error("Please specify features using the --features option.", USAGE_ERROR);
   }

   if ((features->goal || features->theory || (features->offset_problem != -1)) && !problem_file)
   {
      Error("Please specify the problem file using the --problem option.", USAGE_ERROR);
   }
   
   return state;
}
 
void print_help(FILE* out)
{
   fprintf(out, "\n\
Usage: enigmatic-features [options] clauses.p\n\
\n\
Make ENIGMA features from TPTP cnf clauses.\n\
\n");
   PrintOptions(stdout, opts, NULL);
}

/*---------------------------------------------------------------------*/
/*                        End of File                                  */
/*---------------------------------------------------------------------*/

