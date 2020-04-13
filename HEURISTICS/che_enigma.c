/*-----------------------------------------------------------------------

File  : che_feature.c

Author: Jan Jakubuv

Contents
 
  Common functions for ENIGMA.

  Copyright 1998, 1999 by the author.
  This code is released under the GNU General Public Licence and
  the GNU Lesser General Public License.
  See the file COPYING in the main E directory for details..
  Run "eprover -h" for contact information.

Changes

<1> Sat Feb  4 13:33:11 CET 2017
    New


-----------------------------------------------------------------------*/

#include "che_enigma.h"
#include "clb_regmem.h"



/*---------------------------------------------------------------------*/
/*                        Global Variables                             */
/*---------------------------------------------------------------------*/

char*  EmbFileName = NULL;

/*---------------------------------------------------------------------*/
/*                      Forward Declarations                           */
/*---------------------------------------------------------------------*/


/*---------------------------------------------------------------------*/
/*                         Internal Functions                          */
/*---------------------------------------------------------------------*/

/*
static int string_compare(const void* p1, const void* p2)
{
   const char* s1 = ((const IntOrP*)p1)->p_val;
   const char* s2 = ((const IntOrP*)p2)->p_val;
   return -strcmp(s1, s2);
}
*/

static char* fcode_string(FunCode f_code, Enigmap_p enigmap)
{
   static char str[128];
   static char postfix[16];
   
   assert(fcode > 0);
   
   bool is_skolem = false;
   char* name = SigFindName(enigmap->sig, f_code);
   if (name[0] == 'e') // be fast
   {
      if ((strncmp(name, "esk", 3) == 0) || (strncmp(name, "epred", 5) == 0))
      {
         is_skolem = true;
      }
   }
      
   if (is_skolem || (enigmap->version & EFArity))
   {
      int arity = SigFindArity(enigmap->sig, f_code);
      char prefix = SigIsPredicate(enigmap->sig, f_code) ? 'p' : 'f'; 
      char* sk = (is_skolem) ? ENIGMA_SKO : "";
      //char* conj = (FuncQueryProp(&(enigmap->sig->f_info[f_code]), FPInConjecture)) ? "c" : "";
      postfix[0] = '\0';
      if (enigmap->version & EFSine)
      {
         if (f_code <= enigmap->symb_count) 
         {
            snprintf(postfix, 16, "^%ld", enigmap->symb_rank[f_code]);
         }
         else
         {
            sprintf(postfix, "^?");
            //printf("Unknown symbol: %s\n", name);
         }
      }

      sprintf(str, "%s%c%d%s", sk, prefix, arity, postfix);
      StrTree_p node = StrTreeUpdate(&enigmap->name_cache, str, (IntOrP)0L, (IntOrP)0L);
      return node->key; // node->key is now a ("global") copy of str
   }
   else
   {
      return name;
   }
}

static char* top_symbol_string(Term_p term, Enigmap_p enigmap)
{

   if (TermIsVar(term)) 
   {
      return ENIGMA_VAR;
   }
   return fcode_string(term->f_code, enigmap);
}

static unsigned long feature_sdbm(char* str)
{
   unsigned long hash = 0;
   int c;

   while ((c = *str++))
   {
      hash = c + (hash << 6) + (hash << 16) - hash;
   }

   return hash;
}

static long feature_hash(char* str, long base)
{
   return 1 + (feature_sdbm(str) % base);
}

static void feature_increase(
   char* str,
   int inc,
   NumTree_p* counts, 
   Enigmap_p enigmap, 
   int* len)
{
   if (inc == 0)
   {
      return;
   }
   long fid = 0;

   if (enigmap->version & EFHashing)
   {
      fid = feature_hash(str, enigmap->feature_count);
   }
   else
   {
      StrTree_p snode = StrTreeFind(&enigmap->feature_map, str);
      if (snode) 
      {
         fid = snode->val1.i_val;
      }
      else
      {
         //Warning("ENIGMA: Unknown feature \"%s\" skipped.", str);
         return;
      }
   }

   NumTree_p cnode = NumTreeFind(counts, fid);
   if (cnode) 
   {
      cnode->val1.i_val += inc;
   }
   else 
   {
      cnode = NumTreeCellAllocEmpty();
      cnode->key = fid;
      cnode->val1.i_val = inc;
      NumTreeInsert(counts, cnode);
      (*len)++;
   }

   if (enigmap->collect_stats)
   {  
      StrTree_p snode = StrTreeFind(&enigmap->stats, str);
      if (snode)
      {
         snode->val2.i_val += 1;
      }
      else
      {  
         StrTreeStore(&enigmap->stats, str, (IntOrP)fid, (IntOrP)1L);
      }
      //StrTreeUpdate(&enigmap->stats, str, cnode->val1, cnode->val2);
   }
}

static void feature_symbol_increase(
   char* prefix,
   char* fname,
   int inc,
   NumTree_p* counts, 
   Enigmap_p enigmap, 
   int* len)
{
   if (inc == 0)
   {
      return;
   }

   DStr_p str = DStrAlloc();
   DStrAppendStr(str, prefix);
   DStrAppendStr(str, fname);
   feature_increase(DStrView(str), inc, counts, enigmap, len);
   DStrFree(str);
}

static int features_term_collect(
   NumTree_p* counts, 
   Term_p term, 
   Enigmap_p enigmap, 
   char* sym1, 
   char* sym2)
{
   char* sym3 = top_symbol_string(term, enigmap);
   int len = 0;

   // verticals
   if (enigmap->version & EFVertical)
   {
      DStr_p vstr = FeaturesGetTermVertical(sym1, sym2, sym3);
      feature_increase(DStrView(vstr), 1, counts, enigmap, &len);
      DStrFree(vstr);
   }
   
   if (TermIsVar(term)||(TermIsConst(term))) { return len; }
   for (int i=0; i<term->arity; i++)
   {
      len += features_term_collect(counts, term->args[i], enigmap, sym2, sym3);
   }

   // horizontals
   if (enigmap->version & EFHorizontal)
   {
      DStr_p hstr = FeaturesGetTermHorizontal(sym3, term, enigmap);
      feature_increase(DStrView(hstr), 1, counts, enigmap, &len);
      DStrFree(hstr);
   }

   return len;
}

static void features_term_variables(
   Enigmap_p enigmap,
   NumTree_p* stat,
   Term_p term,
   int* distinct,
   int offset)
{
   FunCode f_code = term->f_code;
   if (TermIsVar(term))
   {
      f_code -= offset;
   }
   else if (f_code <= enigmap->sig->internal_symbols)
   {
      f_code = 0L; // ignore internal symbols
   }
   
   if (f_code != 0)
   {
      NumTree_p vnode = NumTreeFind(stat, f_code);
      if (vnode)
      {
         vnode->val1.i_val += 1;
      }
      else
      {
         vnode = NumTreeCellAllocEmpty();
         vnode->key = f_code;
         vnode->val1.i_val = 1;
         NumTreeInsert(stat, vnode);
         if (TermIsVar(term) && distinct)
         {
            (*distinct)++;
         }
      }
   }

   if (!TermIsVar(term))
   {
      for (int i=0; i<term->arity; i++)
      {
         features_term_variables(enigmap, stat, term->args[i], distinct, offset);
      }
   }
}

/*---------------------------------------------------------------------*/
/*                         Exported Functions                          */
/*---------------------------------------------------------------------*/

/* This is old version which puts arguments in reverse (or sorts them)
DStr_p FeaturesGetTermHorizontal(char* top, Term_p term, Enigmap_p enigmap)
{
   DStr_p str = DStrAlloc();
   PStack_p args = PStackAlloc();
   for (int i=0; i<term->arity; i++)
   {
      PStackPushP(args, top_symbol_string(term->args[i], enigmap));
   }
   //PStackSort(args, string_compare);
   DStrAppendStr(str, top);
   while (!PStackEmpty(args))
   {  
      DStrAppendChar(str, '.');
      DStrAppendStr(str, PStackPopP(args));
   }
   PStackFree(args);
   return str;
}
*/

DStr_p FeaturesGetTermHorizontal(char* top, Term_p term, Enigmap_p enigmap)
{
   DStr_p str = DStrAlloc();
   DStrAppendStr(str, top);
   for (int i=0; i<term->arity; i++)
   {
      DStrAppendChar(str, '.');
      DStrAppendStr(str, top_symbol_string(term->args[i], enigmap));
   }
   return str;
}

DStr_p FeaturesGetTermVertical(char* sym1, char* sym2, char* sym3)
{
   DStr_p str = DStrAlloc();
   DStrAppendStr(str, sym1);
   DStrAppendChar(str, ':');
   DStrAppendStr(str, sym2);
   DStrAppendChar(str, ':');
   DStrAppendStr(str, sym3);
   return str;
}

DStr_p FeaturesGetEqHorizontal(Term_p lterm, Term_p rterm, Enigmap_p enigmap)
{
   DStr_p str = DStrAlloc();
   char* lstr = top_symbol_string(lterm, enigmap);
   char* rstr = top_symbol_string(rterm, enigmap);
   bool cond = (strcmp(lstr, rstr) < 0);
   DStrAppendStr(str, ENIGMA_EQ);
   DStrAppendChar(str, '.');
   DStrAppendStr(str, cond ? lstr : rstr);
   DStrAppendChar(str, '.');
   DStrAppendStr(str, cond ? rstr : lstr);
   return str;
}

Enigmap_p EnigmapAlloc(void)
{
   Enigmap_p res = EnigmapCellAlloc();

   res->sig = NULL;
   res->feature_map = NULL;
   res->version = EFNone;
   res->feature_count = 0L;
   res->collect_stats = false;
   res->stats = NULL;
   res->name_cache = NULL;
   res->symb_rank = NULL;
   res->symb_count = 0L;

   return res;
}

void EnigmapFree(Enigmap_p junk)
{
   StrTreeFree(junk->feature_map);
   if (junk->stats)
   {
      StrTreeFree(junk->stats);
   }
   if (junk->name_cache)
   {
      StrTreeFree(junk->name_cache);
   }
   if (junk->symb_rank)
   {
      SizeFree(junk->symb_rank, junk->symb_count*sizeof(long));
   }
   
   EnigmapCellFree(junk);
}

Enigmap_p EnigmapLoad(char* features_filename, Sig_p sig)
{
   Enigmap_p enigmap = EnigmapAlloc();
   long count = 0;

   enigmap->sig = sig;
   enigmap->feature_map = NULL;
   
   Scanner_p in = CreateScanner(StreamTypeFile, features_filename, true, NULL);
   ScannerSetFormat(in, TSTPFormat);                                             
   enigmap->version = EFAll;
   if (TestInpId(in, "version"))
   {
      AcceptInpId(in, "version");
      AcceptInpTok(in, OpenBracket);
      CheckInpTok(in, String);
      enigmap->version = ParseEnigmaFeaturesSpec(AktToken(in)->literal->string);
      NextToken(in);
      AcceptInpTok(in, CloseBracket);
      AcceptInpTok(in, Fullstop);
   }

   if (enigmap->version & EFHashing)
   {
      AcceptInpId(in, "hash_base");
      AcceptInpTok(in, OpenBracket);
      ParseNumString(in);
      enigmap->feature_count = atoi(in->accu->string);
      AcceptInpTok(in, CloseBracket);
      AcceptInpTok(in, Fullstop);
      // skip the rest of the file (buckets collisions info)
      DestroyScanner(in);
   }
   else
   {
      while (TestInpId(in, "feature"))
      {
         AcceptInpId(in, "feature");
         AcceptInpTok(in, OpenBracket);
         ParseNumString(in);
         long fid = atoi(in->accu->string);
         AcceptInpTok(in, Comma);

         CheckInpTok(in, String);
         StrTree_p cell = StrTreeCellAllocEmpty();
         cell->key = DStrCopyCore(AktToken(in)->literal);
         cell->val1.i_val = fid;
         StrTreeInsert(&enigmap->feature_map, cell);
         count++;
         //printf("%ld ... %s\n", fid, DStrCopyCore(AktToken(in)->literal));
         
         NextToken(in);
         AcceptInpTok(in, CloseBracket);
         AcceptInpTok(in, Fullstop);
      }
      CheckInpTok(in, NoToken);
      DestroyScanner(in);

      enigmap->feature_count = count;
   }
   
   return enigmap;
}

void EnigmapFillProblemFeatures(Enigmap_p enigmap, ClauseSet_p axioms)
{
   SpecFeature_p spec = SpecFeatureCellAlloc();
   SpecFeaturesCompute(spec, axioms, enigmap->sig);

   enigmap->problem_features[0] = spec->goals;
   enigmap->problem_features[1] = spec->axioms;
   enigmap->problem_features[2] = spec->clauses;
   enigmap->problem_features[3] = spec->literals;
   enigmap->problem_features[4] = spec->term_cells;
   enigmap->problem_features[5] = spec->unitgoals;
   enigmap->problem_features[6] = spec->unitaxioms;
   enigmap->problem_features[7] = spec->horngoals;
   enigmap->problem_features[8] = spec->hornaxioms;
   enigmap->problem_features[9] = spec->eq_clauses;
   enigmap->problem_features[10] = spec->peq_clauses;
   enigmap->problem_features[11] = spec->groundunitaxioms;
   enigmap->problem_features[12] = spec->groundgoals;
   enigmap->problem_features[13] = spec->groundpositiveaxioms;
   enigmap->problem_features[14] = spec->positiveaxioms;
   enigmap->problem_features[15] = spec->ng_unit_axioms_part;
   enigmap->problem_features[16] = spec->ground_positive_axioms_part;
   enigmap->problem_features[17] = spec->max_fun_arity;
   enigmap->problem_features[18] = spec->avg_fun_arity;
   enigmap->problem_features[19] = spec->sum_fun_arity;
   enigmap->problem_features[20] = spec->clause_max_depth;
   enigmap->problem_features[21] = spec->clause_avg_depth;
   
   SpecFeatureCellFree(spec);
}
      
void conjecture_symbols_term(Enigmap_p enigmap, Term_p term)
{
   if (TermIsVar(term))
   {
      return;
   }

   FuncSetProp(&(enigmap->sig->f_info[term->f_code]), FPInConjecture);

   for (int i=0; i<term->arity; i++)
   {
      conjecture_symbols_term(enigmap, term->args[i]);
   }
}

void EnigmapMarkConjectureSymbols(Enigmap_p enigmap, Clause_p clause)
{
   for (Eqn_p lit = clause->literals; lit; lit = lit->next)
   {
      conjecture_symbols_term(enigmap, lit->lterm);
      conjecture_symbols_term(enigmap, lit->rterm);
   }
}

EnigmaFeatures ParseEnigmaFeaturesSpec(char *spec)
{
   EnigmaFeatures enigma_features = EFNone;
   while (*spec) 
   {
      switch (*spec) 
      {
         case 'V': enigma_features |= EFVertical; break;
         case 'H': enigma_features |= EFHorizontal; break;
         case 'S': enigma_features |= EFSymbols; break;
         case 'L': enigma_features |= EFLengths; break;
         case 'C': enigma_features |= EFConjecture; break;
         case 'W': enigma_features |= EFProofWatch; break;
         case 'X': enigma_features |= EFVariables; break;
         case 'h': enigma_features |= EFHashing; break;
         case 'A': enigma_features |= EFArity; break;
         case 'P': enigma_features |= EFProblem; break;
         case 'I': enigma_features |= EFSine; break;
         case '"': break;
         default:
                   Error("Invalid Enigma features specifier '%c'. Valid characters are 'VHSLCWXAP'.",
                         USAGE_ERROR, *spec);
                   break;
      }
      spec++;
   }
   return enigma_features;
}

int FeaturesClauseExtend(NumTree_p* counts, Clause_p clause, Enigmap_p enigmap)
{
   int len = 0;
   DStr_p hstr;

   for(Eqn_p lit = clause->literals; lit; lit = lit->next)
   {
      char* sym1 = EqnIsPositive(lit)?ENIGMA_POS:ENIGMA_NEG;
      if (lit->rterm->f_code == SIG_TRUE_CODE)
      {
         // verticals & horizontals
         char* sym2 = top_symbol_string(lit->lterm, enigmap); 
         for (int i=0; i<lit->lterm->arity; i++) // here we ignore prop. constants
         {
            len += features_term_collect(counts, lit->lterm->args[i], enigmap, sym1, sym2);
         }

         // top-level horizontal
         if ((enigmap->version & EFHorizontal) && (lit->lterm->arity > 0))
         {
            hstr = FeaturesGetTermHorizontal(sym2, lit->lterm, enigmap);
            feature_increase(DStrView(hstr), 1, counts, enigmap, &len);
            DStrFree(hstr);
         }
      }
      else
      {
         // verticals & horizontals
         char* sym2 = ENIGMA_EQ;
         len += features_term_collect(counts, lit->lterm, enigmap, sym1, sym2);
         len += features_term_collect(counts, lit->rterm, enigmap, sym1, sym2);

         // top-level horizontal
         if (enigmap->version & EFHorizontal)
         {
            hstr = FeaturesGetEqHorizontal(lit->lterm, lit->rterm, enigmap);
            feature_increase(DStrView(hstr), 1, counts, enigmap, &len);
            DStrFree(hstr);
         }
      }
   }

   return len;
}

void FeaturesClauseVariablesExtend(
   Enigmap_p enigmap,
   NumTree_p* stat,
   Clause_p clause,
   int* distinct,
   int offset)
{
   for(Eqn_p lit = clause->literals; lit; lit = lit->next)
   {
      features_term_variables(enigmap, stat, lit->lterm, distinct, offset);
      features_term_variables(enigmap, stat, lit->rterm, distinct, offset);
   }
}

/*
   out[0]: distinct variable count
   out[1]: variable occurances count
   out[2]: unique (non-shared) variables count
   out[3]: shared variables count
   out[4]: max1 (the occurence count of the most occuring variable)
   out[5]: max2 ( --- second most --- )
   out[6]: max3 ( --- third most --- )
   out[7]: min1 (the occurence count of the least occuring variable)
   out[8]: min2 ( --- second --- )
   out[9]: min3 ( --- third --- )
*/
void FeaturesClauseVariablesStat(
   NumTree_p* stat,
   long* out,
   bool pos_keys)
{
   PStack_p stack;
   NumTree_p vnode;

   out[7] = 65536;
   out[8] = 65536;
   out[9] = 65536;

   stack = NumTreeTraverseInit(*stat);
   while ((vnode = NumTreeTraverseNext(stack)))
   {
      if ((pos_keys && (vnode->key <= 0)) || ((!pos_keys) && (vnode->key >= 0)))
      {
         continue;
      }
      out[0]++;
      out[1] += vnode->val1.i_val;
      if (vnode->val1.i_val == 1)
      {
         out[2]++;
      }
      else
      {
         out[3]++;
      }
      // update maximums
      if (vnode->val1.i_val >= out[4])
      {
         out[6] = out[5];
         out[5] = out[4];
         out[4] = vnode->val1.i_val;
      }
      else if (vnode->val1.i_val >= out[5])
      {
         out[6] = out[5];
         out[5] = vnode->val1.i_val;
      }
      else if (vnode->val1.i_val >= out[6])
      {
         out[6] = vnode->val1.i_val;
      }
      // update minimums
      if (vnode->val1.i_val <= out[7])
      {
         out[9] = out[8];
         out[8] = out[7];
         out[7] = vnode->val1.i_val;
      }
      else if (vnode->val1.i_val <= out[8])
      {
         out[9] = out[8];
         out[8] = vnode->val1.i_val;
      }
      else if (vnode->val1.i_val <= out[9])
      {
         out[9] = vnode->val1.i_val;
      }
   }
   NumTreeTraverseExit(stack);
   
   out[7] = (out[7] == 65536) ? 0 : out[7];
   out[8] = (out[8] == 65536) ? 0 : out[8];
   out[9] = (out[9] == 65536) ? 0 : out[9];
}

void FeaturesAddVariables(NumTree_p* counts, NumTree_p* varstat, Enigmap_p enigmap, int *len)
{
   long vars[10];

   // variable statistics
   if (enigmap->version & EFVariables)
   {
      for (int i=0; i<10; i++) { vars[i] = 0L; }
      FeaturesClauseVariablesStat(varstat, vars, false);

      feature_increase("!x!COUNT", vars[0], counts, enigmap, len);
      feature_increase("!x!OCCUR", vars[1], counts, enigmap, len);
      feature_increase("!x!UNIQ", vars[2], counts, enigmap, len);
      feature_increase("!x!SHARE", vars[3], counts, enigmap, len);
      feature_increase("!x!MAX1", vars[4], counts, enigmap, len);
      feature_increase("!x!MAX2", vars[5], counts, enigmap, len);
      feature_increase("!x!MAX3", vars[6], counts, enigmap, len);
      feature_increase("!x!MIN1", vars[7], counts, enigmap, len);
      feature_increase("!x!MIN2", vars[8], counts, enigmap, len);
      feature_increase("!x!MIN3", vars[9], counts, enigmap, len);
   }
   // symbol names statistics
   if (enigmap->version & EFVariables)
   {
      for (int i=0; i<10; i++) { vars[i] = 0L; }
      FeaturesClauseVariablesStat(varstat, vars, true);

      feature_increase("!n!COUNT", vars[0], counts, enigmap, len);
      feature_increase("!n!OCCUR", vars[1], counts, enigmap, len);
      feature_increase("!n!UNIQ", vars[2], counts, enigmap, len);
      feature_increase("!n!SHARE", vars[3], counts, enigmap, len);
      feature_increase("!n!MAX1", vars[4], counts, enigmap, len);
      feature_increase("!n!MAX2", vars[5], counts, enigmap, len);
      feature_increase("!n!MAX3", vars[6], counts, enigmap, len);
      feature_increase("!n!MIN1", vars[7], counts, enigmap, len);
      feature_increase("!n!MIN2", vars[8], counts, enigmap, len);
      feature_increase("!n!MIN3", vars[9], counts, enigmap, len);
   }

   NumTreeFree(*varstat);
   *varstat = NULL;
}

      
void FeaturesAddClauseStatic(NumTree_p* counts, Clause_p clause, Enigmap_p enigmap, int *len, 
         NumTree_p* varstat, int* varoffset)
{
   static long* vec = NULL;
   static size_t size = 0;

   if (!vec)
   {
      size = (4*(enigmap->sig->f_count+1))*sizeof(long);
      vec = RegMemAlloc(size);
   }
   vec = RegMemProvide(vec, &size, (4*(enigmap->sig->f_count+1))*sizeof(long)); // when sig changes
   for (int i=0; i<4*(enigmap->sig->f_count+1); i++) { vec[i] = 0L; }

   if (enigmap->version & EFLengths)
   {
      feature_increase("!LEN", (long)ClauseWeight(clause,1,1,1,1,1,false), counts, enigmap, len);
      feature_increase("!POS", clause->pos_lit_no, counts, enigmap, len);
      feature_increase("!NEG", clause->neg_lit_no, counts, enigmap, len);
   }

   if (enigmap->version & EFSymbols)
   {
      PStack_p mod_stack = PStackAlloc();
      ClauseAddSymbolFeatures(clause, mod_stack, vec);
      PStackFree(mod_stack);
     
      for (long f=enigmap->sig->internal_symbols+1; f<=enigmap->sig->f_count; f++)
      {
         char* fname = fcode_string(f, enigmap);
         if ((strlen(fname)>3) && ((strncmp(fname, "esk", 3) == 0) || (strncmp(fname, "epred", 5) == 0)))
         {
            continue;
         }

         if (vec[4*f+0] > 0)
         {
            feature_symbol_increase("#+", fname, vec[4*f+0], counts, enigmap, len);
            feature_symbol_increase("%+", fname, vec[4*f+1]+1, counts, enigmap, len);
         }
         if (vec[4*f+2] > 0)
         {
            feature_symbol_increase("#-", fname, vec[4*f+2], counts, enigmap, len);
            feature_symbol_increase("%-", fname, vec[4*f+3]+1, counts, enigmap, len);
         }
      }
   }

   if (varoffset && (enigmap->version & EFVariables))
   { 
      int distinct = 0;
      FeaturesClauseVariablesExtend(enigmap, varstat, clause, &distinct, *varoffset);
      (*varoffset) += (2 * distinct);
   }
}

NumTree_p FeaturesClauseCollect(Clause_p clause, Enigmap_p enigmap, int* len)
{
   NumTree_p counts = NULL;
   NumTree_p varstat = NULL;
   int varoffset = 0;

   *len = FeaturesClauseExtend(&counts, clause, enigmap);
   FeaturesAddClauseStatic(&counts, clause, enigmap, len, &varstat, &varoffset);
   FeaturesAddVariables(&counts, &varstat, enigmap, len);
   return counts;
}

void FeaturesSvdTranslate(DMat matUt, double* sing, 
   struct feature_node* in, struct feature_node* out)
{
   int i,j;
   double dot;

   for (i=0; i<matUt->rows; i++)
   {
      dot = 0.0;
      j = 0;
      while (in[j].index != -1) 
      {
         dot += matUt->value[i][in[j].index-1] * in[j].value;
         j++;
      }

      out[i].index = i+1;
      out[i].value = dot / sing[i];
   }
   out[i].index = -1;
}

/*---------------------------------------------------------------------*/
/*                        End of File                                  */
/*---------------------------------------------------------------------*/

