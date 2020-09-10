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


#include "che_enigmaticdata.h"



/*---------------------------------------------------------------------*/
/*                        Global Variables                             */
/*---------------------------------------------------------------------*/

#define INFO_SETTING(out,name,params,key) fprintf(out,"setting(\"%s:%s\", %ld).\n", name, #key, (long)params->key)

#define RESET_ARRAY(array,len) for (i=0;i<len;i++) { array[i] = 0; }

#define PRINT_INT(key,val) if (val) fprintf(out, "%ld:%ld ", key, val)
#define PRINT_FLOAT(key,val) if (val) fprintf(out, "%ld:%.2f ", key, val)

/* ENIGMA feature names (efn) */
char* efn_lengths[] = {
   "len",
   "lits",
   "pos",
   "neg",
   "depth",
   "width",
   "avg_depth",
   "pos_eqs",
   "neg_eqs",
   "pos_atoms",
   "neg_atoms",
   NULL
};

char* efn_problem[] = {
   "goals",
   "axioms",
   "clauses",
   "literals",
   "term_cells",
   "unitgoals",
   "unitaxioms",
   "horngoals",
   "hornaxioms",
   "eq_clauses",
   "peq_clauses",
   "groundunitaxioms",
   "groundgoals",
   "groundpositiveaxioms",
   "positiveaxioms",
   "ng_unit_axioms_part",
   "ground_positive_axioms_part",
   "max_fun_arity",
   "avg_fun_arity",
   "sum_fun_arity",
   "clause_max_depth",
   "clause_avg_depth",
   NULL
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
         parse_maybe(spec, ';');
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
         case 'e': params->use_eprover = true; break;
         case 'a': params->anonymous = true; break;
         case 'x': parse_one(spec, 'c', &params->count_var, &default_count); break;
         case 's': parse_one(spec, 'c', &params->count_sym, &default_count); break;
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

static long params_offsets(EnigmaticParams_p params, long start)
{
   long cur = start;

   params_offset(params->use_len, &params->offset_len, EFC_LEN, &cur);
   params_offset(params->count_var != -1, &params->offset_var, EFC_VAR(params), &cur);
   params_offset(params->count_sym != -1, &params->offset_sym, EFC_SYM(params), &cur);
   params_offset(params->use_eprover, &params->offset_eprover, EFC_EPROVER, &cur);
   params_offset(params->base_horiz != -1, &params->offset_horiz, EFC_HORIZ(params), &cur);
   params_offset(params->base_vert != -1, &params->offset_vert, EFC_VERT(params), &cur);
   params_offset(params->base_count != -1, &params->offset_count, EFC_COUNT(params), &cur);
   params_offset(params->base_depth != -1, &params->offset_depth, EFC_DEPTH(params), &cur);

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
   info_offset(out, name, "eprover", params->offset_eprover);
   info_offset(out, name, "horiz", params->offset_horiz);
   info_offset(out, name, "vert", params->offset_vert);
   info_offset(out, name, "count", params->offset_count);
   info_offset(out, name, "depth", params->offset_depth);
}

static void info_settings(FILE* out, char* name, EnigmaticParams_p params)
{
   if (!params)
   {
      return;
   }
   INFO_SETTING(out, name, params, features);
   INFO_SETTING(out, name, params, anonymous);
   INFO_SETTING(out, name, params, use_len);
   INFO_SETTING(out, name, params, use_eprover);
   INFO_SETTING(out, name, params, count_var);
   INFO_SETTING(out, name, params, count_sym);
   INFO_SETTING(out, name, params, length_vert);
   INFO_SETTING(out, name, params, base_vert);
   INFO_SETTING(out, name, params, base_horiz);
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

static void names_range(FILE* out, char* prefix, char* class, long offset, long size)
{
   if (offset == -1)
   {
      return;
   }
   if (class)
   {
      fprintf(out, "feature_begin(%ld, \"%s:%s\").\n", offset, prefix, class);
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
   names_range(out, name, "var", params->offset_var, EFC_VAR(params)); 
   names_range(out, name, "sym", params->offset_sym, EFC_SYM(params)); 
   names_range(out, name, "eprover", params->offset_eprover, EFC_EPROVER); 
   names_range(out, name, "horiz", params->offset_horiz, EFC_HORIZ(params)); 
   names_range(out, name, "vert", params->offset_vert, EFC_VERT(params)); 
   names_range(out, name, "count", params->offset_count, EFC_COUNT(params)); 
   names_range(out, name, "depth", params->offset_depth, EFC_DEPTH(params)); 
}

static void print_enigmatic_clause(FILE* out, EnigmaticClause_p clause)
{
   if (!clause)
   {
      return;
   }
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

EnigmaticParams_p EnigmaticParamsAlloc(void)
{
   EnigmaticParams_p params = EnigmaticParamsCellAlloc();
   params->features = -1;
   params->anonymous = false;
   params->use_len = false;
   params->use_eprover = false;
   params->count_var = -1;
   params->count_sym = -1;
   params->length_vert = -1;
   params->base_vert = -1;
   params->base_horiz = -1;
   params->base_count = -1;
   params->base_depth = -1;
   params->offset_len = -1;
   params->offset_var = -1;
   params->offset_sym = -1;
   params->offset_eprover = -1;
   params->offset_horiz = -1;
   params->offset_vert = -1;
   params->offset_count = -1;
   params->offset_depth = -1;
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
   params->use_len = source->use_len;
   params->use_eprover = source->use_eprover;
   params->count_var = source->count_var;
   params->count_sym = source->count_sym;
   params->length_vert = source->length_vert;
   params->base_vert = source->base_vert;
   params->base_horiz = source->base_horiz;
   params->base_count = source->base_count;
   params->base_depth = source->base_depth;
   params->offset_len = source->offset_len;
   params->offset_var = source->offset_var;
   params->offset_sym = source->offset_sym;
   params->offset_eprover = source->offset_eprover;
   params->offset_horiz = source->offset_horiz;
   params->offset_vert = source->offset_vert;
   params->offset_count = source->offset_count;
   params->offset_depth = source->offset_depth;
   return params;
}

EnigmaticFeatures_p EnigmaticFeaturesAlloc(void)
{
   EnigmaticFeatures_p features = EnigmaticFeaturesCellAlloc();
   features->offset_clause = -1;
   features->offset_goal = -1;
   features->offset_theory = -1;
   features->offset_problem = -1;
   features->offset_proofwatch = -1;
   features->clause = NULL;
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
   if (junk->goal)
   {
      EnigmaticParamsFree(junk->goal);
   }
   if (junk->theory)
   {
      EnigmaticParamsFree(junk->theory);
   }
   EnigmaticFeaturesCellFree(junk);
}

EnigmaticFeatures_p EnigmaticFeaturesParse(char* spec)
{
   EnigmaticFeatures_p features = EnigmaticFeaturesAlloc();

   while (*spec)
   {
      switch (*spec)
      {
         case 'C': 
            parse_expect(&spec, 'C');
            features->offset_clause = 0;
            features->clause = parse_block(&spec); 
            break;
         case 'G': 
            parse_expect(&spec, 'G');
            features->offset_goal = 0;
            features->goal = parse_block(&spec); 
            break;
         case 'T': 
            parse_expect(&spec, 'T');
            features->theory = parse_block(&spec); 
            features->offset_theory = 0;
            break;
         case 'P':
            parse_expect(&spec, 'P');
            features->offset_problem = 0;
            break;
         case 'W':
            parse_expect(&spec, 'W');
            features->offset_proofwatch = 0;
            break;
         default:
            Error("ENIGMA: Invalid feature specifier (expected block name, have '%s').",
                  USAGE_ERROR, spec);
            break;
      }
      parse_maybe(&spec, ':');
   }

   // set defaults if not specified
   if (!features->clause)
   {
      features->clause = EnigmaticParamsAlloc();
      features->clause->use_len = true;
   }
   if ((features->offset_goal == 0) && (!features->goal))
   {
      features->goal = EnigmaticParamsCopy(features->clause);
   }
   if ((features->offset_theory == 0) && (!features->theory))
   {
      features->theory = EnigmaticParamsCopy(features->clause);
   }

   // update offsets
   long idx = ENIGMATIC_FIRST;
   if (features->offset_clause == 0) 
   {
      features->offset_clause = idx;
      idx += params_offsets(features->clause, idx);
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

   return features;
}

void EnigmaticFeaturesInfo(FILE* out, EnigmaticFeatures_p features, char* spec)
{
   if (spec)
   {
      fprintf(out, "features(\"%s\").\n", spec);
   }
   info_offset(out, "clause", NULL, features->offset_clause);
   info_offset(out, "goal", NULL, features->offset_goal);
   info_offset(out, "theory", NULL, features->offset_theory);
   info_offset(out, "problem", NULL, features->offset_problem);
   info_offset(out, "proofwatch", NULL, features->offset_proofwatch);
   info_suboffsets(out, "clause", features->clause);
   info_suboffsets(out, "goal", features->goal);
   info_suboffsets(out, "theory", features->theory);
   info_settings(out, "clause", features->clause);
   info_settings(out, "goal", features->goal);
   info_settings(out, "theory", features->theory);
}

void EnigmaticFeaturesMap(FILE* out, EnigmaticFeatures_p features)
{
   names_clauses(out, "clause", features->clause, features->offset_clause);
   names_clauses(out, "goal", features->goal, features->offset_goal);
   names_clauses(out, "theory", features->theory, features->offset_theory);
   names_array(out, "problem", features->offset_problem, efn_problem, EBS_PROBLEM);
   //names_proofwatch(out, offset_proofwatch);
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
   enigma->vert = NULL;
   enigma->horiz = NULL;
   enigma->counts = NULL;
   enigma->depths = NULL;

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
   if (junk->vert) { NumTreeFree(junk->vert); }
   if (junk->horiz) { NumTreeFree(junk->horiz); }
   if (junk->counts) { NumTreeFree(junk->counts); }
   if (junk->depths) { NumTreeFree(junk->depths); }
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
   enigma->avg_depth = 0;
   enigma->pos_eqs = 0;
   enigma->neg_eqs = 0;
   enigma->pos_atoms = 0;
   enigma->neg_atoms = 0;
   if (enigma->vert)
   {
      NumTreeFree(enigma->vert);
      enigma->vert = NULL;
   }
   if (enigma->horiz)
   {
      NumTreeFree(enigma->horiz);
      enigma->horiz = NULL;
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
}

EnigmaticVector_p EnigmaticVectorAlloc(EnigmaticFeatures_p features)
{
   EnigmaticVector_p vector = EnigmaticVectorCellAlloc();
   vector->features = features;
   vector->clause = NULL;
   vector->goal = NULL;
   vector->theory = NULL;
   if (features->offset_clause != -1)
   {
      vector->clause = EnigmaticClauseAlloc(features->clause);
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
   EnigmaticVectorCellFree(junk);
}

EnigmaticInfo_p EnigmaticInfoAlloc()
{
   EnigmaticInfo_p info = EnigmaticInfoCellAlloc();
   info->occs = NULL;
   info->sig = NULL;
   return info;
}

void EnigmaticInfoReset(EnigmaticInfo_p info)
{
   if (info->occs)
   {
      NumTreeFree(info->occs);
      info->occs = NULL;
   }
}

void EnigmaticInfoFree(EnigmaticInfo_p junk)
{
   EnigmaticInfoReset(junk);
   EnigmaticInfoCellFree(junk);
}

void PrintEnigmaticVector(FILE* out, EnigmaticVector_p vector)
{
   print_enigmatic_clause(out, vector->clause);
   print_enigmatic_clause(out, vector->goal);
   print_enigmatic_clause(out, vector->theory);
}


/*---------------------------------------------------------------------*/
/*                        End of File                                  */
/*---------------------------------------------------------------------*/

