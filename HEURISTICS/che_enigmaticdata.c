/*-----------------------------------------------------------------------

File  : che_enigmaticdata.c

Author: Stephan Schultz, AI4REASON

Contents
 
  Copyright 2020 by the authors.
  This code is released under the GNU General Public Licence.
  See the file COPYING in the main CLIB directory for details.
  Run "eprover -h" for contact information.

Changes

<1> Fri 10 Apr 2020 11:14:30 PM CEST

-----------------------------------------------------------------------*/


#include <math.h>
#include "che_enigmaticdata.h"



/*---------------------------------------------------------------------*/
/*                        Global Variables                             */
/*---------------------------------------------------------------------*/

#define INFO_SETTING(out,name,params,key) fprintf(out,"setting(\"%s:%s\", %ld).\n", name, #key, (long)params->key)


#define PRINT_INT(key,val) if (val) fprintf(out, "%ld:%ld ", key, val)
#define PRINT_FLOAT(key,val) if (val) fprintf(out, "%ld:%.2f ", key, val)

/* ENIGMA feature names (efn) */
char* efn_lengths[EFC_LEN] = {
   "len",
   "lits",
   "pos",
   "neg",
   "depth",
   "width",
   "avg_lit_depth",
   "avg_lit_width",
   "avg_lit_len",
   "pos_eqs",
   "neg_eqs",
   "pos_atoms",
   "neg_atoms",
   "vars_count",
   "vars_occs",
   "vars_unique",
   "vars_shared",
   "preds_count",
   "preds_occs",
   "preds_unique",
   "preds_shared",
   "funcs_count",
   "funcs_occs",
   "funcs_unique",
   "funcs_shared"
};

char* efn_problem[EBS_PROBLEM] = {
   "axiomtypes",
   "goaltypes",
   "eq_content",
   "ng_unit_content",
   "ground_positive_content",
   "goals_are_ground",
   "set_clause_size",
   "set_literal_size",
   "set_termcell_size",
   "max_fun_ar_class",
   "avg_fun_ar_class",
   "sum_fun_ar_class",
   "max_depth_class",
   "clauses",
   "goals",
   "axioms",
   "literals",
   "term_cells",
   "clause_max_depth",
   "clause_avg_lit_depth",
   "unit",
   "unitgoals",
   "unitaxioms",
   "horn",
   "horngoals",
   "hornaxioms",
   "eq_clauses",
   "peq_clauses",
   "groundunitaxioms",
   "positiveaxioms",
   "groundpositiveaxioms",
   "groundgoals",
   "ng_unit_axioms_part",
   "ground_positive_axioms_part",
   "max_fun_arity",
   "avg_fun_arity",
   "sum_fun_arity",
   "max_pred_arity",
   "avg_pred_arity",
   "sum_pred_arity",
   "fun_const_count",
   "fun_nonconst_count",
   "pred_nonconst_count"
};

char* efn_prios[EFC_PRIOS] = {
   "PreferGroundGoals",
   "PreferUnitGroundGoals",
   "PreferGround",
   "PreferNonGround",
   "PreferGoals",
   "PreferNonGoals",
   "PreferMixed",
   "PreferPositive",
   "PreferNegative",
   "PreferUnits",
   "PreferNonEqUnits",
   "PreferDemods",
   "PreferNonUnits",
   "ByLiteralNumber",
   "ByNegLitDist",
   "GoalDifficulty",
   "PreferHorn",
   "PreferNonHorn",
   "PreferUnitAndNonEq",
   "DeferNonUnitMaxPosEq",
   "ByPosLitNo",
   "ByHornDist"
};

ClausePrioFun ecb_prios[EFC_PRIOS] = {
   PrioFunPreferGroundGoals,
   PrioFunPreferUnitGroundGoals,
   PrioFunPreferGround,
   PrioFunPreferNonGround,
   PrioFunPreferGoals,
   PrioFunPreferNonGoals,
   PrioFunPreferMixed,
   PrioFunPreferPositive,
   PrioFunPreferNegative,
   PrioFunPreferUnits,
   PrioFunPreferNonEqUnits,
   PrioFunPreferDemods,
   PrioFunPreferNonUnits,
   PrioFunByLiteralNumber,
   PrioFunByNegLitDist,
   PrioFunGoalDifficulty,
   PrioFunPreferHorn,
   PrioFunPreferNonHorn,
   PrioFunPreferUnitAndNonEq,
   PrioFunDeferNonUnitMaxPosEq,
   PrioFunByPosLitNo,
   PrioFunByHornDist
};

/*---------------------------------------------------------------------*/
/*                      Forward Declarations                           */
/*---------------------------------------------------------------------*/

/*---------------------------------------------------------------------*/
/*                         Internal Functions                          */
/*---------------------------------------------------------------------*/

static void parse_expect(char** spec, char token)
{
   if (**spec != token)
   {
      Error("ENIGMA: Invalid feature specifier (expected '%c', have '%s')", SYNTAX_ERROR, token, *spec); 
   }
   (*spec)++;
}

static void parse_maybe(char** spec, char token)
{
   if (**spec == token)
   {
      (*spec)++;
   }
}

static void parse_keyval(char** spec, char key, long* val, long* def)
{
   if (**spec == key)
   {
      parse_expect(spec, key);
      parse_expect(spec, '=');
      *val = strtol(*spec, spec, 10);
      *def = *val;
   }
   else
   {
      *val = *def;
   }
}

static void parse_one(char** spec, char key, long* val, long* def)
{
   if (**spec == '[') 
   {
      parse_expect(spec, '[');
      parse_keyval(spec, key, val, def);
      parse_expect(spec, ']');
   }
   else
   {
      *val = *def;
   }
}

static void parse_vert(
   char** spec, 
   EnigmaticParams_p params, 
   long* default_length, 
   long* default_base)
{
   if (**spec == '[')
   {
      parse_expect(spec, '[');
      while (**spec != ']')
      {
         switch (**spec) 
         {
            case 'l': parse_keyval(spec, 'l', &params->length_vert, default_length); break;
            case 'b': parse_keyval(spec, 'b', &params->base_vert, default_base); break;
            default:
            Error("ENIGMA: Invalid feature specifier (expected vertical parameters ('l','b'), have '%s').",
                  USAGE_ERROR, *spec);
            break;
         }
         parse_maybe(spec, ':');
      }
      parse_expect(spec, ']');
   }

   // set to defaults if not set above   
   if (params->length_vert == -1) 
   { 
      params->length_vert = *default_length; 
   }
   if (params->base_vert == -1) 
   { 
      params->base_vert = *default_base; 
   }
}

static EnigmaticParams_p parse_block(char** spec)
{
   static long default_count = EDV_COUNT;
   static long default_length = EDV_LENGTH;
   static long default_base = EDV_BASE;

   if (**spec != '(') 
   { 
      return NULL; 
   }
   parse_expect(spec, '(');
   if (**spec == ')')
   {
      parse_expect(spec, ')');
      return NULL; // empty params '()'
   }
   EnigmaticParams_p params = EnigmaticParamsAlloc();
   while (**spec != ')')
   {
      char arg = **spec;
      (*spec)++;
      switch (arg) 
      {
         case 'l': params->use_len = true; break;
         case 'p': params->use_prios = true; break;
         case 'a': params->anonymous = true; break;
         case 'u': params->unified_hashing = true; break;
         case 'x': parse_one(spec, 'c', &params->count_var, &default_count); break;
         case 's': parse_one(spec, 'c', &params->count_sym, &default_count); break;
         case 'r': parse_one(spec, 'c', &params->count_arity, &default_count); break;
         case 'v': parse_vert(spec, params, &default_length, &default_base); break;
         case 'h': parse_one(spec, 'b', &params->base_horiz, &default_base); break;
         case 'c': parse_one(spec, 'b', &params->base_count, &default_base); break;
         case 'd': parse_one(spec, 'b', &params->base_depth, &default_base); break;
         default:
            Error("ENIGMA: Invalid feature specifier (unknown argument name '%c').",
                  USAGE_ERROR, arg);
            break;
      }
      parse_maybe(spec, ',');
   }
   parse_expect(spec, ')');
   return params;
}

static void params_offset(bool cond, long* offset, long len, long* cur)
{
   if (cond)
   {
      *offset = *cur;
      *cur += len;
   }
   else
   {
      *offset = -1;
   }
}

static void params_offset_set(bool cond, long* offset, long cur)
{
   if (cond)
   {
      *offset = cur;
   }
   else
   {
      *offset = -1;
   }
}

static long params_offsets(EnigmaticParams_p params, long start)
{
   long cur = start;
   params_offset(params->use_len, &params->offset_len, EFC_LEN, &cur);
   params_offset(params->count_var != -1, &params->offset_var, EFC_VAR(params), &cur);
   params_offset(params->count_sym != -1, &params->offset_sym, EFC_SYM(params), &cur);
   params_offset(params->count_arity != -1, &params->offset_arity, EFC_ARITY(params), &cur);
   params_offset(params->use_prios, &params->offset_prios, EFC_PRIOS, &cur);
   if (!params->unified_hashing)
   {
      params_offset(params->base_horiz != -1, &params->offset_horiz, EFC_HORIZ(params), &cur);
      params_offset(params->base_vert  != -1, &params->offset_vert,  EFC_VERT(params),  &cur);
      params_offset(params->base_count != -1, &params->offset_count, EFC_COUNT(params), &cur);
      params_offset(params->base_depth != -1, &params->offset_depth, EFC_DEPTH(params), &cur);
   }
   else
   {
      long base = EFC_UNIFIED(params);
      params_offset_set(params->base_horiz != -1, &params->offset_horiz, cur);
      params_offset_set(params->base_vert  != -1, &params->offset_vert,  cur);
      params_offset_set(params->base_count != -1, &params->offset_count, cur);
      params_offset_set(params->base_depth != -1, &params->offset_depth, cur);
      params_offset(base != -1, &params->offset_unified, base, &cur);
   }

   params->features = cur - start;

   return params->features;
}


static void info_offset(FILE* out, char* name, char* subname, long offset)
{
   if (offset != -1)
   {
      if (subname)
      {
         fprintf(out, "suboffset(\"%s:%s\", %ld).\n", name, subname, offset);
      }
      else
      {
         fprintf(out, "offset(\"%s\", %ld).\n", name, offset);
      }
   }
}

static void info_suboffsets(FILE* out, char* name, EnigmaticParams_p params)
{
   if (!params)
   {
      return;
   }
   info_offset(out, name, "len", params->offset_len);
   info_offset(out, name, "var", params->offset_var);
   info_offset(out, name, "sym", params->offset_sym);
   info_offset(out, name, "arity", params->offset_arity);
   info_offset(out, name, "prio", params->offset_prios);
   info_offset(out, name, "horiz", params->offset_horiz);
   info_offset(out, name, "vert", params->offset_vert);
   info_offset(out, name, "count", params->offset_count);
   info_offset(out, name, "depth", params->offset_depth);
   info_offset(out, name, "unified", params->offset_unified);
}

static void info_settings(FILE* out, char* name, EnigmaticParams_p params)
{
   if (!params)
   {
      return;
   }
   INFO_SETTING(out, name, params, features);
   INFO_SETTING(out, name, params, anonymous);
   INFO_SETTING(out, name, params, unified_hashing);
   INFO_SETTING(out, name, params, use_len);
   INFO_SETTING(out, name, params, use_prios);
   INFO_SETTING(out, name, params, count_var);
   INFO_SETTING(out, name, params, count_sym);
   INFO_SETTING(out, name, params, count_arity);
   INFO_SETTING(out, name, params, length_vert);
   INFO_SETTING(out, name, params, base_horiz);
   INFO_SETTING(out, name, params, base_vert);
   INFO_SETTING(out, name, params, base_count);
   INFO_SETTING(out, name, params, base_depth);
}

static void names_array(FILE* out, char* prefix, long offset, char* names[], long size)
{
   if (offset == -1)
   {
      return;
   }
   for (int i=0; i<size; i++)
   {
      fprintf(out, "feature_name(%ld, \"%s:%s\").\n", offset+i, prefix, names[i]);
   }

}

static void names_range(FILE* out, char* prefix, char* class, long offset, long size, long count)
{
   if (offset == -1)
   {
      return;
   }
   if (class)
   {
      fprintf(out, "feature_begin(%ld, \"%s:%s\").\n", offset, prefix, class);
      if (count > 0)
      {
         for (int i=0; i<(size/count); i++)
         {
            fprintf(out, "feature_start(%ld, \"%s:%s:%d\").\n", offset+(i*count), prefix, class, i);
         }
      }
      fprintf(out, "feature_end(%ld, \"%s:%s\").\n", offset+size-1, prefix, class);
   }
   else
   {
      fprintf(out, "feature_begin(%ld, \"%s\").\n", offset, prefix);
      fprintf(out, "feature_end(%ld, \"%s\").\n", offset+size-1, prefix);
}
}

static void names_clauses(FILE* out, char* name, EnigmaticParams_p params, long offset)
{
   if (offset == -1)
   {
      return;
   }
   names_array(out, name, params->offset_len, efn_lengths, EFC_LEN);
   names_range(out, name, "var", params->offset_var, EFC_VAR(params), params->count_var); 
   names_range(out, name, "sym", params->offset_sym, EFC_SYM(params), params->count_sym); 
   names_range(out, name, "arity", params->offset_arity, EFC_ARITY(params), params->count_arity);
   names_array(out, name, params->offset_prios, efn_prios, EFC_PRIOS);
   names_range(out, name, "horiz", params->offset_horiz, EFC_HORIZ(params), 0); 
   names_range(out, name, "vert", params->offset_vert, EFC_VERT(params), 0); 
   names_range(out, name, "count", params->offset_count, EFC_COUNT(params), 0); 
   names_range(out, name, "depth", params->offset_depth, EFC_DEPTH(params), 0); 
   names_range(out, name, "unified", params->offset_unified, EFC_UNIFIED(params), 0); 
}

static void fill_print(void* data, long key, float val)
{
   PrintKeyVal(data, key, val);
}

static void fill_lengths(FillFunc set, void* data, EnigmaticClause_p clause)
{
   if (clause->params->use_len)
   {
      long offset = clause->params->offset_len;
      set(data, offset+0,  clause->len);
      set(data, offset+1,  clause->lits);
      set(data, offset+2,  clause->pos);
      set(data, offset+3,  clause->neg);
      set(data, offset+4,  clause->depth);
      set(data, offset+5,  clause->width);
      set(data, offset+6,  clause->avg_lit_depth);
      set(data, offset+7,  clause->avg_lit_width);
      set(data, offset+8,  clause->avg_lit_len);
      set(data, offset+9,  clause->pos_eqs);
      set(data, offset+10, clause->neg_eqs);
      set(data, offset+11, clause->pos_atoms);
      set(data, offset+12, clause->neg_atoms);
      set(data, offset+13, clause->vars_count);
      set(data, offset+14, clause->vars_occs);
      set(data, offset+15, clause->vars_unique);
      set(data, offset+16, clause->vars_shared);
      set(data, offset+17, clause->preds_count);
      set(data, offset+18, clause->preds_occs);
      set(data, offset+19, clause->preds_unique);
      set(data, offset+20, clause->preds_shared);
      set(data, offset+21, clause->funcs_count);
      set(data, offset+22, clause->funcs_occs);
      set(data, offset+23, clause->funcs_unique);
      set(data, offset+24, clause->funcs_shared);
   }
}

static void fill_array_int(FillFunc set, void* data, long* array, int len, long offset)
{
   for (int i=0; i<len; i++)
   {
      set(data, offset+i, (float)array[i]);
   }
}

static void fill_array_float(FillFunc set, void* data, float* array, int len, long offset)
{
   for (int i=0; i<len; i++)
   {
      set(data, offset+i, array[i]);
   }
}

static void fill_hist(FillFunc set, void* data, long offset, long len, long* hist, long* count, float* rat)
{
   fill_array_int(set, data, hist, len, offset);
   fill_array_int(set, data, count, len, offset+1*len);
   fill_array_float(set, data, rat, len, offset+2*len);
}

static void fill_arities(FillFunc set, void* data, long offset, long len, EnigmaticClause_p clause)
{
   fill_array_int(set, data, clause->arity_pred_hist, len, offset);
   fill_array_int(set, data, clause->arity_func_hist, len, offset+1*len);
   fill_array_int(set, data, clause->arity_pred_occs, len, offset+2*len);
   fill_array_int(set, data, clause->arity_func_occs, len, offset+3*len);
}

static void fill_hists(FillFunc set, void* data, EnigmaticClause_p clause)
{
   // variables
   fill_hist(set, data, clause->params->offset_var, clause->params->count_var,
      clause->var_hist, clause->var_count, clause->var_rat);
   // predicate symbols
   fill_hist(set, data, clause->params->offset_sym, clause->params->count_sym,
      clause->pred_hist, clause->pred_count, clause->pred_rat);
   // function symbols
   fill_hist(set, data, clause->params->offset_sym+(3*clause->params->count_sym), clause->params->count_sym,
      clause->func_hist, clause->func_count, clause->func_rat);
   // arity histograms
   fill_arities(set, data, clause->params->offset_arity, clause->params->count_arity, clause);
}

static void fill_hashes(FillFunc set, void* data, NumTree_p map, long offset)
{
   NumTree_p node;
   if (!map) { return; }
   PStack_p stack = NumTreeTraverseInit(map);
   while ((node = NumTreeTraverseNext(stack)))
   {
      set(data, offset + node->key, node->val1.i_val);
   }
   NumTreeTraverseExit(stack);
}

static void fill_prios(FillFunc set, void* data, EnigmaticClause_p clause)
{
   if (!clause->params->use_prios) { return; }

   for (int i=0; i<EFC_PRIOS; i++)
   {
      set(data, clause->params->offset_prios + i, clause->prios[i]);
   }
}

static void fill_clause(FillFunc set, void* data, EnigmaticClause_p clause)
{
   if (!clause)
   {
      return;
   }
   fill_lengths(set, data, clause);
   fill_hists(set, data, clause);
   fill_prios(set, data, clause);
   if (!clause->params->unified_hashing)
   {
      fill_hashes(set, data, clause->horiz, clause->params->offset_horiz);
      fill_hashes(set, data, clause->vert, clause->params->offset_vert);
      fill_hashes(set, data, clause->counts, clause->params->offset_count);
      fill_hashes(set, data, clause->depths, clause->params->offset_depth);
   }
   else
   {
      fill_hashes(set, data, clause->unified, clause->params->offset_unified);
   }
}

static void fill_problem(FillFunc set, void* data, EnigmaticVector_p vector)
{
   long offset = vector->features->offset_problem;
   if (offset < 0) { return; }

   for (int i=0; i<EBS_PROBLEM; i++)
   {
      set(data, offset + i, vector->problem_features[i]);
   }
}

/*---------------------------------------------------------------------*/
/*                         Exported Functions                          */
/*---------------------------------------------------------------------*/

EnigmaticParams_p EnigmaticParamsAlloc(void)
{
   EnigmaticParams_p params = EnigmaticParamsCellAlloc();
   params->features = -1;
   params->anonymous = false;
   params->unified_hashing = false;
   params->use_len = false;
   params->use_prios = false;
   params->count_var = -1;
   params->count_sym = -1;
   params->count_arity = -1;
   params->length_vert = -1;
   params->base_horiz = -1;
   params->base_vert = -1;
   params->base_count = -1;
   params->base_depth = -1;
   params->offset_len = -1;
   params->offset_var = -1;
   params->offset_sym = -1;
   params->offset_arity = -1;
   params->offset_prios = -1;
   params->offset_horiz = -1;
   params->offset_vert = -1;
   params->offset_count = -1;
   params->offset_depth = -1;
   params->offset_unified = -1;
   return params;
}

void EnigmaticParamsFree(EnigmaticParams_p junk)
{
   EnigmaticParamsCellFree(junk);
}

EnigmaticParams_p EnigmaticParamsCopy(EnigmaticParams_p source)
{
   EnigmaticParams_p params = EnigmaticParamsCellAlloc();
   params->features = source->features;
   params->anonymous = source->anonymous;
   params->unified_hashing = source->unified_hashing;
   params->use_len = source->use_len;
   params->use_prios = source->use_prios;
   params->count_var = source->count_var;
   params->count_sym = source->count_sym;
   params->count_arity = source->count_arity;
   params->length_vert = source->length_vert;
   params->base_horiz = source->base_horiz;
   params->base_vert = source->base_vert;
   params->base_count = source->base_count;
   params->base_depth = source->base_depth;
   params->offset_len = source->offset_len;
   params->offset_var = source->offset_var;
   params->offset_sym = source->offset_sym;
   params->offset_arity = source->offset_arity;
   params->offset_prios = source->offset_prios;
   params->offset_horiz = source->offset_horiz;
   params->offset_vert = source->offset_vert;
   params->offset_count = source->offset_count;
   params->offset_depth = source->offset_depth;
   params->offset_unified = source->offset_unified;
   return params;
}

EnigmaticFeatures_p EnigmaticFeaturesAlloc(void)
{
   EnigmaticFeatures_p features = EnigmaticFeaturesCellAlloc();
   features->spec = NULL;
   features->offset_clause = -1;
   features->offset_co_parent = -1;
   features->offset_goal = -1;
   features->offset_theory = -1;
   features->offset_problem = -1;
   features->offset_proofwatch = -1;
   features->clause = NULL;
   features->co_parent = NULL;
   features->goal = NULL;
   features->theory = NULL;
   return features;
}

void EnigmaticFeaturesFree(EnigmaticFeatures_p junk)
{
   if (junk->clause)
   {
      EnigmaticParamsFree(junk->clause);
   }
   if (junk->co_parent)
   {
	  EnigmaticParamsFree(junk->co_parent);
   }
   if (junk->goal)
   {
      EnigmaticParamsFree(junk->goal);
   }
   if (junk->theory)
   {
      EnigmaticParamsFree(junk->theory);
   }
   if (junk->spec)
   {
      DStrFree(junk->spec);
   }
   EnigmaticFeaturesCellFree(junk);
}

EnigmaticFeatures_p EnigmaticFeaturesParse(char* spec)
{
   EnigmaticFeatures_p features = EnigmaticFeaturesAlloc();
   EnigmaticParams_p defaults = NULL;
   features->spec = DStrAlloc();
   DStrAppendStr(features->spec, spec);

   if (spec[0] != 'C')
   {
      Error("ENIGMATIC: Feature specifier must start with 'C'.", OTHER_ERROR);
   }

   while (*spec)
   {
      switch (*spec)
      {
         case 'C': 
            if (features->offset_clause == 0) { Error("ENIGMATIC: Multiple '%c' blocks are not allowed.", OTHER_ERROR, *spec); }
            parse_expect(&spec, 'C');
            features->offset_clause = 0;
            features->clause = parse_block(&spec); 
            if (!features->clause)
            {
               features->clause = EnigmaticParamsAlloc();
               features->clause->use_len = true;
            }
            defaults = features->clause;
            break;
         case 'G': 
            if (features->offset_goal == 0) { Error("ENIGMATIC: Multiple '%c' blocks are not allowed.", OTHER_ERROR, *spec); }
            parse_expect(&spec, 'G');
            features->offset_goal = 0;
            features->goal = parse_block(&spec); 
            if ((!features->goal) && (defaults)) 
            { 
               features->goal = EnigmaticParamsCopy(defaults); 
            }
            defaults = features->goal;
            break;
         case 'T': 
            if (features->offset_theory == 0) { Error("ENIGMATIC: Multiple '%c' blocks are not allowed.", OTHER_ERROR, *spec); }
            parse_expect(&spec, 'T');
            features->offset_theory = 0;
            features->theory = parse_block(&spec); 
            if ((!features->theory) && (defaults)) 
            { 
               features->theory = EnigmaticParamsCopy(defaults); 
            }
            defaults = features->theory;
            break;
         case 'P':
            if (features->offset_problem == 0) { Error("ENIGMATIC: Multiple '%c' blocks are not allowed.", OTHER_ERROR, *spec); }
            parse_expect(&spec, 'P');
            features->offset_problem = 0;
            break;
         case 'W':
            if (features->offset_proofwatch == 0) { Error("ENIGMATIC: Multiple '%c' blocks are not allowed.", OTHER_ERROR, *spec); }
            parse_expect(&spec, 'W');
            features->offset_proofwatch = 0;
            break;
         case 'F':
			if (features->offset_co_parent == 0) { Error("ENIGMATIC: Multiple '%c' blocks are not allowed.", OTHER_ERROR, *spec); }
			parse_expect(&spec, 'F');
			features->offset_co_parent = 0;
			features->co_parent = parse_block(&spec);
			if ((!features->co_parent) && (defaults))
			{
				features->co_parent = EnigmaticParamsCopy(defaults);
			}
			defaults = features->co_parent;
			break;
         default:
            Error("ENIGMA: Invalid feature specifier (expected block name, have '%s').",
                  USAGE_ERROR, spec);
            break;
      }
      parse_maybe(&spec, ':');
   }

   // update offsets
   long idx = ENIGMATIC_FIRST;
   if (features->offset_clause == 0) 
   {
      features->offset_clause = idx;
      idx += params_offsets(features->clause, idx);
   }
   if (features->offset_co_parent == 0)
   {
      features->offset_co_parent = idx;
      idx += params_offsets(features->co_parent, idx);
   }
   if (features->offset_goal == 0) 
   {
      features->offset_goal = idx;
      idx += params_offsets(features->goal, idx);
   }
   if (features->offset_theory == 0) 
   {
      features->offset_theory = idx;
      idx += params_offsets(features->theory, idx);
   }
   if (features->offset_problem == 0)
   {
      features->offset_problem = idx;
      idx += EBS_PROBLEM;
   }
   if (features->offset_proofwatch == 0)
   {
      features->offset_proofwatch = idx;
      // idx += ??? // watchlist size is unknown
   }
   features->count = idx + 1; // plus one for terminator feature

   return features;
}

EnigmaticFeatures_p EnigmaticFeaturesLoad(char* filename)
{
   EnigmaticFeatures_p features;
   Scanner_p in = CreateScanner(StreamTypeFile, filename, true, NULL, true);
   ScannerSetFormat(in, TSTPFormat);
   AcceptInpId(in, "features");
   AcceptInpTok(in, OpenBracket);
   CheckInpTok(in, String);
   // strip '"'
   char* str = DStrView(AktToken(in)->literal);
   str[strlen(str)-1] = '\0';
   str = &str[1];
   features = EnigmaticFeaturesParse(str);
   NextToken(in);
   AcceptInpTok(in, CloseBracket);
   AcceptInpTok(in, Fullstop);
   DestroyScanner(in);
   return features;
}

EnigmaticClause_p EnigmaticClauseAlloc(EnigmaticParams_p params)
{
   EnigmaticClause_p enigma = EnigmaticClauseCellAlloc();
   enigma->params = params;
   enigma->var_hist = NULL;
   enigma->var_count = NULL;
   enigma->var_rat = NULL;
   enigma->func_hist = NULL;
   enigma->pred_hist = NULL;
   enigma->func_count = NULL;
   enigma->pred_count = NULL;
   enigma->func_rat = NULL;
   enigma->pred_rat = NULL;
   enigma->horiz = NULL;
   enigma->vert = NULL;
   enigma->counts = NULL;
   enigma->depths = NULL;
   enigma->unified = NULL;

   if (params->count_var > 0)
   {
      enigma->var_hist = SizeMalloc(params->count_var*sizeof(long));
      enigma->var_count = SizeMalloc(params->count_var*sizeof(long));
      enigma->var_rat = SizeMalloc(params->count_var*sizeof(float));
   }
   if (params->count_sym > 0)
   {
      enigma->func_hist = SizeMalloc(params->count_sym*sizeof(long));
      enigma->func_count = SizeMalloc(params->count_sym*sizeof(long));
      enigma->func_rat = SizeMalloc(params->count_sym*sizeof(float));
      enigma->pred_hist = SizeMalloc(params->count_sym*sizeof(long));
      enigma->pred_count = SizeMalloc(params->count_sym*sizeof(long));
      enigma->pred_rat = SizeMalloc(params->count_sym*sizeof(float));
   }
   if (params->count_arity > 0)
   {
      enigma->arity_func_hist = SizeMalloc(params->count_arity*sizeof(long));
      enigma->arity_pred_hist = SizeMalloc(params->count_arity*sizeof(long));
      enigma->arity_func_occs = SizeMalloc(params->count_arity*sizeof(long));
      enigma->arity_pred_occs = SizeMalloc(params->count_arity*sizeof(long));
   }

   EnigmaticClauseReset(enigma);
   return enigma;
}

void EnigmaticClauseFree(EnigmaticClause_p junk)
{
   if (junk->params->count_var > 0)
   {
      SizeFree(junk->var_hist, junk->params->count_var*sizeof(long));
      SizeFree(junk->var_count, junk->params->count_var*sizeof(long));
      SizeFree(junk->var_rat, junk->params->count_var*sizeof(float));
   }
   if (junk->params->count_sym > 0)
   {
      SizeFree(junk->func_hist, junk->params->count_sym*sizeof(long));
      SizeFree(junk->func_count, junk->params->count_sym*sizeof(long));
      SizeFree(junk->func_rat, junk->params->count_sym*sizeof(float));
      SizeFree(junk->pred_hist, junk->params->count_sym*sizeof(long));
      SizeFree(junk->pred_count, junk->params->count_sym*sizeof(long));
      SizeFree(junk->pred_rat, junk->params->count_sym*sizeof(float));
   }
   if (junk->params->count_arity > 0)
   {
      SizeFree(junk->arity_func_hist, junk->params->count_arity*sizeof(long));
      SizeFree(junk->arity_pred_hist, junk->params->count_arity*sizeof(long));
      SizeFree(junk->arity_func_occs, junk->params->count_arity*sizeof(long));
      SizeFree(junk->arity_pred_occs, junk->params->count_arity*sizeof(long));
   }
   if (junk->horiz) { NumTreeFree(junk->horiz); }
   if (junk->vert) { NumTreeFree(junk->vert); }
   if (junk->counts) { NumTreeFree(junk->counts); }
   if (junk->depths) { NumTreeFree(junk->depths); }
   if (junk->unified) { NumTreeFree(junk->unified); }
   EnigmaticClauseCellFree(junk);
}

void EnigmaticClauseReset(EnigmaticClause_p enigma)
{
   enigma->len = 0;
   enigma->lits = 0;
   enigma->pos = 0;
   enigma->neg = 0;
   enigma->depth = 0;
   enigma->width = 0;
   enigma->avg_lit_depth = 0;
   enigma->avg_lit_width = 0;
   enigma->avg_lit_len = 0;
   enigma->pos_eqs = 0;
   enigma->neg_eqs = 0;
   enigma->pos_atoms = 0;
   enigma->neg_atoms = 0;
   enigma->vars_count = 0;
   enigma->vars_occs = 0;
   enigma->vars_unique = 0;
   enigma->vars_shared = 0;
   enigma->preds_count = 0;
   enigma->preds_occs = 0;
   enigma->preds_unique = 0;
   enigma->preds_shared = 0;
   enigma->funcs_count = 0;
   enigma->funcs_occs = 0;
   enigma->funcs_unique = 0;
   enigma->funcs_shared = 0;

   if (enigma->horiz)
   {
      NumTreeFree(enigma->horiz);
      enigma->horiz = NULL;
   }
   if (enigma->vert)
   {
      NumTreeFree(enigma->vert);
      enigma->vert = NULL;
   }
   if (enigma->counts)
   {
      NumTreeFree(enigma->counts);
      enigma->counts = NULL;
   }
   if (enigma->depths)
   {
      NumTreeFree(enigma->depths);
      enigma->depths = NULL;
   }
   if (enigma->unified)
   {
      NumTreeFree(enigma->unified);
      enigma->unified = NULL;
   }
   int i;
   if (enigma->params->count_var > 0)
   {
      RESET_ARRAY(enigma->var_hist, enigma->params->count_var);
      RESET_ARRAY(enigma->var_count, enigma->params->count_var);
      RESET_ARRAY(enigma->var_rat, enigma->params->count_var);
   }
   if (enigma->params->count_sym > 0)
   {
      RESET_ARRAY(enigma->func_hist, enigma->params->count_sym);
      RESET_ARRAY(enigma->func_count, enigma->params->count_sym);
      RESET_ARRAY(enigma->func_rat, enigma->params->count_sym);
      RESET_ARRAY(enigma->pred_hist, enigma->params->count_sym);
      RESET_ARRAY(enigma->pred_count, enigma->params->count_sym);
      RESET_ARRAY(enigma->pred_rat, enigma->params->count_sym);
   }
   if (enigma->params->count_arity > 0)
   {
      RESET_ARRAY(enigma->arity_func_hist, enigma->params->count_arity);
      RESET_ARRAY(enigma->arity_pred_hist, enigma->params->count_arity);
      RESET_ARRAY(enigma->arity_func_occs, enigma->params->count_arity);
      RESET_ARRAY(enigma->arity_pred_occs, enigma->params->count_arity);
   }
   if (enigma->params->use_prios)
   {
      RESET_ARRAY(enigma->prios, EFC_PRIOS);
   }
}

EnigmaticVector_p EnigmaticVectorAlloc(EnigmaticFeatures_p features)
{
   EnigmaticVector_p vector = EnigmaticVectorCellAlloc();
   vector->features = features;
   vector->clause = NULL;
   vector->goal = NULL;
   vector->theory = NULL;
   vector->co_parent = NULL;
   if (features->offset_clause != -1)
   {
      vector->clause = EnigmaticClauseAlloc(features->clause);
   }
   if (features->offset_co_parent != -1)
   {
	  vector->co_parent = EnigmaticClauseAlloc(features->co_parent);
   }
   if (features->offset_goal != -1)
   {
      vector->goal = EnigmaticClauseAlloc(features->goal);
   }
   if (features->offset_theory != -1)
   {
      vector->theory = EnigmaticClauseAlloc(features->theory);
   }
   int i;
   RESET_ARRAY(vector->problem_features, EBS_PROBLEM);
   return vector;
}


void EnigmaticVectorFree(EnigmaticVector_p junk)
{
   if (junk->features)
   {
      EnigmaticFeaturesFree(junk->features);
   }
   if (junk->clause)
   {
      EnigmaticClauseFree(junk->clause);
   }
   if (junk->goal)
   {
      EnigmaticClauseFree(junk->goal);
   }
   if (junk->theory)
   {
      EnigmaticClauseFree(junk->theory);
   }
   if (junk->co_parent)
      {
         EnigmaticClauseFree(junk->co_parent);
      }
   EnigmaticVectorCellFree(junk);
}

EnigmaticInfo_p EnigmaticInfoAlloc()
{
   EnigmaticInfo_p info = EnigmaticInfoCellAlloc();
   info->occs = NULL;
   info->sig = NULL;
   info->path = PStackAlloc();
   info->symbol_cache = NULL;
   info->collect_hashes = false;
   info->hashes = NULL;
   info->avgs = NULL;
   return info;
}

// reset between clauses; does not reset: symbol_cache and hashes stats
void EnigmaticInfoReset(EnigmaticInfo_p info)
{
   if (info->occs)
   {
      NumTreeFree(info->occs);
      info->occs = NULL;
   }
   PStackReset(info->path);
}

void EnigmaticInfoFree(EnigmaticInfo_p junk)
{
   EnigmaticInfoReset(junk);
   PStackFree(junk->path);
   if (junk->symbol_cache)
   {
      PStack_p stack = NumTreeTraverseInit(junk->symbol_cache);
      NumTree_p node;
      while ((node = NumTreeTraverseNext(stack)))
      {
         DStrFree((DStr_p)node->val1.p_val);
      }
      NumTreeTraverseExit(stack);
      NumTreeFree(junk->symbol_cache);
   }
   if (junk->hashes)
   {
      StrTreeFree(junk->hashes);
   }
   EnigmaticInfoCellFree(junk);
}

EnigmaticModel_p EnigmaticModelAlloc(void)
{
   EnigmaticModel_p model = EnigmaticModelCellAlloc();
   model->handle = NULL;
   model->info = NULL;
   model->vector = NULL;
   return model;
}

void EnigmaticModelFree(EnigmaticModel_p junk)
{
   if (junk->model_filename)
   {
      FREE(junk->model_filename);
   }
   if (junk->features_filename)
   {
      FREE(junk->features_filename);
   }
   if (junk->vector)
   {
      EnigmaticVectorFree(junk->vector);
   }
   if (junk->info)
   {
      EnigmaticInfoFree(junk->info);
   }
   EnigmaticModelCellFree(junk);
}

EnigmaticModel_p EnigmaticModelCreate(char* d_prefix, char* model_name)
{
   EnigmaticModel_p model = EnigmaticModelAlloc();
   char* d_prfx;
   int len_prefix = strlen(d_prefix);
   if (d_prefix[0] == '"') 
   {
      d_prefix[len_prefix-1] = '\0';
      d_prfx = &d_prefix[1];
   }
   else 
   {
      d_prfx = d_prefix;
   }

   char* d_enigma = getenv("ENIGMATIC_ROOT");
   if (!d_enigma) 
   {
      d_enigma = ".";
   }

   DStr_p f_model = DStrAlloc();
   DStrAppendStr(f_model, d_enigma);
   DStrAppendStr(f_model, "/");
   DStrAppendStr(f_model, d_prfx);
   DStrAppendStr(f_model, "/");
   DStrAppendStr(f_model, model_name);
   model->model_filename = SecureStrdup(DStrView(f_model));
   DStrFree(f_model);

   DStr_p f_featmap = DStrAlloc();
   DStrAppendStr(f_featmap, d_enigma);
   DStrAppendStr(f_featmap, "/");
   DStrAppendStr(f_featmap, d_prfx);
   DStrAppendStr(f_featmap, "/");
   DStrAppendStr(f_featmap, "enigma.map");
   model->features_filename = SecureStrdup(DStrView(f_featmap));
   DStrFree(f_featmap);
 
   return model;
}

EnigmaticModel_p EnigmaticWeightParse(Scanner_p in, char* model_name)
{
   char* d_prefix = ParseFilename(in);
   EnigmaticModel_p model = EnigmaticModelCreate(d_prefix, model_name);
   AcceptInpTok(in, Comma);
   model->weight_type = ParseInt(in);
   AcceptInpTok(in, Comma);
   model->threshold = ParseFloat(in);
   FREE(d_prefix);
   return model;
}

void EnigmaticVectorFill(EnigmaticVector_p vector, FillFunc set, void* data)
{
   fill_clause(set, data, vector->clause);
   fill_clause(set, data, vector->co_parent);
   fill_clause(set, data, vector->goal);
   fill_clause(set, data, vector->theory);
   fill_problem(set, data, vector);
   set(data, vector->features->count-1, 23); // terminator feature
}

void PrintKeyVal(FILE* out, long key, float val)
{
   if ((!val) || (isnan(val))) { return; }
   if (ceilf(val) == val)
   {
      fprintf(out, "%ld:%ld ", key, (long)val);
   }
   else
   {
      fprintf(out, "%ld:%.2f ", key, val);
   }
}

void PrintEscapedString(FILE* out, char* str)
{
   fprintf(out, "\"");
   while (*str)
   {
      switch (*str)
      {
         case '"':
            fprintf(out, "\\\"");
            break;
         case '\\':
            fprintf(out, "\\\\");
            break;
         default:
            fprintf(out, "%c", *str);
            break;
      }
      str++;
   }
   fprintf(out, "\"");
}

void PrintEnigmaticVector(FILE* out, EnigmaticVector_p vector)
{
   EnigmaticVectorFill(vector, fill_print, out);
}

void PrintEnigmaticFeaturesMap(FILE* out, EnigmaticFeatures_p features)
{
   names_clauses(out, "clause", features->clause, features->offset_clause);
   names_clauses(out, "co_parent", features->co_parent, features->offset_co_parent);
   names_clauses(out, "goal", features->goal, features->offset_goal);
   names_clauses(out, "theory", features->theory, features->offset_theory);
   names_array(out, "problem", features->offset_problem, efn_problem, EBS_PROBLEM);
   //names_proofwatch(out, offset_proofwatch);
}

void PrintEnigmaticFeaturesInfo(FILE* out, EnigmaticFeatures_p features)
{
   fprintf(out, "features(\"%s\").\n", DStrView(features->spec));
   fprintf(out, "count(%ld).\n", features->count);
   
   info_offset(out, "clause", NULL, features->offset_clause);
   info_offset(out, "co_parent", NULL, features->offset_co_parent);
   info_offset(out, "goal", NULL, features->offset_goal);
   info_offset(out, "theory", NULL, features->offset_theory);
   info_offset(out, "problem", NULL, features->offset_problem);
   info_offset(out, "proofwatch", NULL, features->offset_proofwatch);
   
   info_suboffsets(out, "clause", features->clause);
   info_suboffsets(out, "co_parent", features->co_parent);
   info_suboffsets(out, "goal", features->goal);
   info_suboffsets(out, "theory", features->theory);
   
   info_settings(out, "clause", features->clause);
   info_settings(out, "co_parent", features->co_parent);
   info_settings(out, "goal", features->goal);
   info_settings(out, "theory", features->theory);
}

void PrintEnigmaticBuckets(FILE* out, EnigmaticInfo_p info)
{
   if (!info->hashes) { fprintf(out, "{}\n"); return; }
   StrTree_p node;
   PStack_p stack = StrTreeTraverseInit(info->hashes);
   fprintf(out, "{\n");
   node = StrTreeTraverseNext(stack);
   while (node)
   {
      fprintf(out, "\t");
      PrintEscapedString(out, node->key);
      fprintf(out, ": [%ld, %ld]", node->val1.i_val, node->val2.i_val);
      node = StrTreeTraverseNext(stack);
      fprintf(out, "%s\n", (node ? "," : ""));
   }
   fprintf(out, "}\n");
   StrTreeTraverseExit(stack);
}


/*---------------------------------------------------------------------*/
/*                        End of File                                  */
/*---------------------------------------------------------------------*/

