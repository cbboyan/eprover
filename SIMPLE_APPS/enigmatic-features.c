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
#include <ccl_formula_wrapper.h>
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
   OPT_OUTPUT_MAP,
   OPT_FREE_NUMBERS,
   OPT_PROBLEM,
   OPT_FEATURES,
   OPT_PREFIX,
   OPT_PREFIX_POS,
   OPT_PREFIX_NEG,
   OPT_AVG,
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
   {OPT_OUTPUT_MAP,
      'm', "output-map",
      ReqArg, NULL,
      "Writes Enigma feature info mapping into the named file."},
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
   {OPT_PREFIX,
      '\0', "prefix",
      ReqArg, NULL,
      "Prefix the clauses feature vectors with the provided string."},
   {OPT_PREFIX_POS,
      '\0', "prefix-pos",
      NoArg, NULL,
      "Same as --prefix=\"+1 \"."},
   {OPT_PREFIX_NEG,
      '\0', "prefix-neg",
      NoArg, NULL,
      "Same as --prefix=\"-0 \"."},
   {OPT_AVG,
      '\0', "avg",
      NoArg, NULL,
      "Compute one average vector and output it instead of the clause vectors."},
   {OPT_NOOPT,
      '\0', NULL,
      NoArg, NULL,
      NULL}
};

char *outname = NULL;
FILE* MapOut = NULL;
FunctionProperties free_symb_prop = FPIgnoreProps;
EnigmaticFeatures_p features;
char* problem_file = NULL;
ProblemType problemType = PROBLEM_FO;
bool app_encode = false;
char* prefix = "";
bool compute_avg = false;

/*---------------------------------------------------------------------*/
/*                      Forward Declarations                           */
/*---------------------------------------------------------------------*/

CLState_p process_options(int argc, char* argv[]);
void print_help(FILE* out);

/*---------------------------------------------------------------------*/
/*                         Internal Functions                          */
/*---------------------------------------------------------------------*/

static void process_problem(char* problem_file, EnigmaticVector_p vector, EnigmaticInfo_p info)
{
   if (!problem_file)
   {
      return;
   }

   Scanner_p in = CreateScanner(StreamTypeFile, problem_file, true, NULL, true);
   ScannerSetFormat(in, TSTPFormat);
   ClauseSet_p wlset = ClauseSetAlloc();
   FormulaSet_p fset = FormulaSetAlloc();
   FormulaAndClauseSetParse(in, fset, wlset, info->bank, NULL, NULL);
   CheckInpTok(in, NoToken);

   EnigmaticInitProblem(vector, info, fset, wlset);

   DestroyScanner(in);
   ClauseSetFreeClauses(wlset);
   ClauseSetFree(wlset);
   FormulaSetFreeFormulas(fset);
   FormulaSetFree(fset);
}

static void fill_avg_sum(void* data, long idx, float val)
{
   EnigmaticInfo_p info = data;
   info->avgs[idx] += val;
}

static void process_clauses(FILE* out, char* filename, EnigmaticVector_p vector, EnigmaticInfo_p info)
{
   Scanner_p in = CreateScanner(StreamTypeFile, filename, true, NULL, true);
   ScannerSetFormat(in, TSTPFormat);
   Clause_p clause;
   WFormula_p formula = NULL;
  
   int count = 0;
   while (TestInpId(in, "input_formula|input_clause|fof|cnf|tff|tcf"))
   {
      if (TestInpId(in, "input_clause|cnf"))
      {
         clause = ClauseParse(in, info->bank);
      }
      else 
      {
         formula = WFormulaParse(in, info->bank);
         clause = EnigmaticFormulaToClause(formula, info);
         WFormulaFree(formula);
      }
      EnigmaticClause(vector->clause, clause, info);
      if (!compute_avg)
      {
         ClausePrint(out, clause, true);
         fprintf(out, "\n");

         fprintf(out, prefix);
         PrintEnigmaticVector(GlobalOut, vector);
         fprintf(out, "\n");
      }
      else
      {
         EnigmaticVectorFill(vector, fill_avg_sum, info);
      }

      count++;
      ClauseFree(clause);
      EnigmaticClauseReset(vector->clause);
   }

   if (compute_avg)
   {
      fprintf(out, prefix);
      for (int i=0; i<features->count; i++)
      {
         PrintKeyVal(out, i, info->avgs[i] / count);
      }
   }

   CheckInpTok(in, NoToken);
   DestroyScanner(in);
}

int main(int argc, char* argv[])
{
   int i;
   InitIO(argv[0]);
   CLState_p args = process_options(argc, argv);
   //SetMemoryLimit(2L*1024*MEGA);
   OutputFormat = TSTPFormat;
   if (outname) { OpenGlobalOut(outname); }
   ProofState_p state = ProofStateAlloc(free_symb_prop);
   EnigmaticVector_p vector = EnigmaticVectorAlloc(features);

   EnigmaticInfo_p info = EnigmaticInfoAlloc();
   info->sig = state->signature;
   info->bank = state->terms;
   info->collect_hashes = true;
   info->avgs = SizeMalloc(features->count*sizeof(float));
   RESET_ARRAY(info->avgs, features->count);

   process_problem(problem_file, vector, info);
   process_clauses(GlobalOut, args->argv[0], vector, info);
  
   if (MapOut)
   {
      PrintEnigmaticFeaturesInfo(MapOut, features);
      PrintEnigmaticFeaturesMap(MapOut, features);
      PrintEnigmaticHashes(MapOut, info);
      fclose(MapOut);
   }
 
   EnigmaticVectorFree(vector);
   SizeFree(info->avgs, features->count*sizeof(float));
   EnigmaticInfoFree(info);
   ProofStateFree(state);
   CLStateFree(args);
   ExitIO();

#ifdef CLB_MEMORY_DEBUG
   RegMemCleanUp();
   MemFlushFreeList();
   MemDebugPrintStats(stdout);
#endif

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
      case OPT_OUTPUT_MAP:
         MapOut = fopen(arg, "w");
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
      case OPT_PREFIX:
         prefix = arg;
         break;
      case OPT_PREFIX_POS:
         prefix = "+1 ";
         break;
      case OPT_PREFIX_NEG:
         prefix = "-0 ";
         break;
      case OPT_AVG:
         compute_avg = true;
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

