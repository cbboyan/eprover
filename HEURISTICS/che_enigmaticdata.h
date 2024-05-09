/*-----------------------------------------------------------------------

File  : che_enigmaticdata.h

Author: Stephan Schultz, AI4REASON

Contents
 
  Copyright 2020 by the authors.
  This code is released under the GNU General Public Licence.
  See the file COPYING in the main CLIB directory for details.
  Run "eprover -h" for contact information.

Changes

<1> Fri 10 Apr 2020 11:14:30 PM CEST

-----------------------------------------------------------------------*/

#ifndef CHE_ENIGMATICDATA

#define CHE_ENIGMATICDATA

#include <ccl_clauses.h>
#include <che_prio_funs.h>

/*---------------------------------------------------------------------*/
/*                    Data type declarations                           */
/*---------------------------------------------------------------------*/

extern char* efn_lengths[];
extern char* efn_problem[];
extern char* efn_prios[];

extern ClausePrioFun ecb_prios[];

/* Enigmatic first feature index */
#define ENIGMATIC_FIRST     0

/* Enigmatic Default Value */
#define EDV_COUNT           6
#define EDV_BASE            1024
#define EDV_LENGTH          3

/* Enigmatic Feature Count */
#define EFC_LEN             25
#define EFC_VAR(params)     (3*(params->count_var))
#define EFC_SYM(params)     (6*(params->count_sym))
#define EFC_ARITY(params)   (4*(params->count_arity))
#define EFC_PRIOS           22
#define EFC_VERT(params)    (params->base_vert)
#define EFC_HORIZ(params)   (params->base_horiz)
#define EFC_COUNT(params)   (params->base_count)
#define EFC_DEPTH(params)   (params->base_depth)
#define EFC_UNIFIED(params) (MAX(MAX((params->base_vert),(params->base_horiz)),MAX((params->base_count),(params->base_depth))))

/* Enigmatic Block Size */
#define EBS_PROBLEM         43

/* Enigmatic Symbol Strings */
#define ENIGMATIC_VAR       "*"
#define ENIGMATIC_POS       "+"
#define ENIGMATIC_NEG       "-"
#define ENIGMATIC_SKO       "?"
#define ENIGMATIC_EQ        "="
#define ENIGMATIC_DB        "^"

#define ENIGMATIC_HORIZ     "."
#define ENIGMATIC_VERT      "|"
#define ENIGMATIC_COUNT     "#"
#define ENIGMATIC_DEPTH     "%"

#define ENIGMATIC_TYPE      ":"
#define ENIGMATIC_ANY       "_"

/* Enigmatic weigths */
#define EW_POS 1.0
#define EW_NEG 10.0
#define EW_WORST 100.0

#define RESET_ARRAY(array,len) for (i=0;i<(len);i++) { (array)[i] = 0; }

typedef struct enigmaticparamscell
{
   long features;
   bool anonymous;
   bool unified_hashing;
   bool use_types;

   bool use_len;
   //bool use_minimal;
   bool use_prios;
   long count_var;
   long count_sym;
   long count_arity;
   long length_vert;
   long base_vert;
   long base_horiz;
   long base_count;
   long base_depth;
   
   long offset_len;
   long offset_var;
   long offset_sym;
   long offset_arity;
   long offset_prios;
   long offset_horiz;
   long offset_vert;
   long offset_count;
   long offset_depth;
   long offset_unified;
} EnigmaticParamsCell, *EnigmaticParams_p;

typedef struct enigmaticfeaturescell
{
   DStr_p spec;
   long count; 

   long offset_clause;
   long offset_goal;
   long offset_theory;
   long offset_problem;
   long offset_proofwatch;
   long offset_co_parent;
   
   EnigmaticParams_p clause;
   EnigmaticParams_p goal;
   EnigmaticParams_p theory;
   EnigmaticParams_p co_parent;
} EnigmaticFeaturesCell, *EnigmaticFeatures_p;

typedef struct enigmaticclausecell
{
   EnigmaticParams_p params; // a pointer copy, do not free!

   // length statistics
   long len;
   long lits;
   long pos;
   long neg;
   long depth;
   long width;
   float avg_lit_depth;
   float avg_lit_len; 
   float avg_lit_width;
   long pos_eqs;
   long neg_eqs;
   long pos_atoms;
   long neg_atoms;
   long vars_count;
   long vars_occs;
   long vars_unique;
   long vars_shared;
   long preds_count;
   long preds_occs;
   long preds_unique;
   long preds_shared;
   long funcs_count;
   long funcs_occs;
   long funcs_unique;
   long funcs_shared;

   long* var_hist;
   long* var_count;
   float* var_rat;
   // symbol statistics
   long* func_hist;
   long* pred_hist;
   long* func_count;
   long* pred_count;
   float* func_rat;
   float* pred_rat;
   // arity statistics
   long* arity_func_hist;
   long* arity_pred_hist;
   long* arity_func_occs;
   long* arity_pred_occs;
   // eprover prio/weights values
   float prios[EFC_PRIOS];
   // vertical features
   NumTree_p vert;
   // horizontal features
   NumTree_p horiz;
   // symbol count statistic features
   NumTree_p counts;
   // symbol depth statistic features
   NumTree_p depths;
   // unified clause hash map
   NumTree_p unified;


} EnigmaticClauseCell, *EnigmaticClause_p;

typedef struct enigmaticvectorcell
{
   EnigmaticFeatures_p features;
   // clause features
   EnigmaticClause_p clause;
   // goal (conjecture) features
   EnigmaticClause_p goal;
   // theory features
   EnigmaticClause_p theory;
   // parent number two's features
   EnigmaticClause_p co_parent;
   // problem features
   float problem_features[EBS_PROBLEM];
   // TODO: proofwatch features

} EnigmaticVectorCell, *EnigmaticVector_p;

/* This is a data structure to encapsulate various data used when 
 * computing features so that passing of a large number of arguments
 * is avoided.
 */
typedef struct enigmaticinfocell
{
   int var_offset;   // variable offset to distinquish variables from different clauses
   int var_distinct; // count of distinct variables
   NumTree_p occs;   // symbol/vars occurrences map :: f_code -> occurrence count
   Sig_p sig;        // signature link
   TB_p bank;        // term bank
   PStack_p path;    // current symbol path with a literal
   bool pos;         // is current literal positive?
   NumTree_p symbol_cache; // stores enigmatized symbol names (skolems & anonymous)
   //NumTree_p hash_cache; // stores hash codes of symbols
   bool collect_hashes;  // collect feature hash statistics in the below:
   StrTree_p hashes;     //   (hash map :: festure string -> feature hashed id)
   float* avgs;          // computing the average vector
} EnigmaticInfoCell, *EnigmaticInfo_p;

typedef struct enigmaticmodelcell
{
   char* model_filename;
   char* features_filename;
   int weight_type;
   double threshold;
   void* handle; // TODO: rename to booster
   EnigmaticInfo_p info;
   EnigmaticVector_p vector;
} EnigmaticModelCell, *EnigmaticModel_p;

typedef void (*FillFunc)(void*, long, float);

typedef double (*PredictFunc)(void*, struct enigmaticmodelcell*);

typedef void (*LoadFunc)(struct enigmaticmodelcell*);

/*---------------------------------------------------------------------*/
/*                Exported Functions and Variables                     */
/*---------------------------------------------------------------------*/


#define EnigmaticParamsCellAlloc() (EnigmaticParamsCell*) \
        SizeMalloc(sizeof(EnigmaticParamsCell))
#define EnigmaticParamsCellFree(junk) \
        SizeFree(junk, sizeof(EnigmaticParamsCell))

#define EnigmaticFeaturesCellAlloc() (EnigmaticFeaturesCell*) \
        SizeMalloc(sizeof(EnigmaticFeaturesCell))
#define EnigmaticFeaturesCellFree(junk) \
        SizeFree(junk, sizeof(EnigmaticFeaturesCell))

#define EnigmaticClauseCellAlloc() (EnigmaticClauseCell*) \
        SizeMalloc(sizeof(EnigmaticClauseCell))
#define EnigmaticClauseCellFree(junk) \
        SizeFree(junk, sizeof(EnigmaticClauseCell))

#define EnigmaticVectorCellAlloc() (EnigmaticVectorCell*) \
        SizeMalloc(sizeof(EnigmaticVectorCell))
#define EnigmaticVectorCellFree(junk) \
        SizeFree(junk, sizeof(EnigmaticVectorCell))

#define EnigmaticInfoCellAlloc() (EnigmaticInfoCell*) \
        SizeMalloc(sizeof(EnigmaticInfoCell))
#define EnigmaticInfoCellFree(junk) \
        SizeFree(junk, sizeof(EnigmaticInfoCell))

#define EnigmaticModelCellAlloc() (EnigmaticModelCell*) \
        SizeMalloc(sizeof(EnigmaticModelCell))
#define EnigmaticModelCellFree(junk) \
        SizeFree(junk, sizeof(EnigmaticModelCell))

EnigmaticParams_p EnigmaticParamsAlloc(void);
void EnigmaticParamsFree(EnigmaticParams_p junk);
EnigmaticParams_p EnigmaticParamsCopy(EnigmaticParams_p source);

EnigmaticFeatures_p EnigmaticFeaturesAlloc(void);
void EnigmaticFeaturesFree(EnigmaticFeatures_p junk);
EnigmaticFeatures_p EnigmaticFeaturesParse(char* spec);
EnigmaticFeatures_p EnigmaticFeaturesLoad(char* filename);

EnigmaticClause_p EnigmaticClauseAlloc(EnigmaticParams_p params);
void EnigmaticClauseFree(EnigmaticClause_p junk);
void EnigmaticClauseReset(EnigmaticClause_p enigma);

EnigmaticVector_p EnigmaticVectorAlloc(EnigmaticFeatures_p features);
void EnigmaticVectorFree(EnigmaticVector_p junk);
void EnigmaticVectorFill(EnigmaticVector_p vector, FillFunc fun, void* data);

EnigmaticInfo_p EnigmaticInfoAlloc();
void EnigmaticInfoReset(EnigmaticInfo_p info);
void EnigmaticInfoFree(EnigmaticInfo_p junk);

EnigmaticModel_p EnigmaticModelAlloc(void);
void EnigmaticModelFree(EnigmaticModel_p junk);
EnigmaticModel_p EnigmaticModelCreate(char* d_prefix, char* model_name);
EnigmaticModel_p EnigmaticWeightParse(Scanner_p in, char* model_name);

void PrintKeyVal(FILE* out, long key, float val);
void PrintEscapedString(FILE* out, char* str);

void PrintEnigmaticVector(FILE* out, EnigmaticVector_p vector);
void PrintEnigmaticFeaturesMap(FILE* out, EnigmaticFeatures_p features);
void PrintEnigmaticFeaturesInfo(FILE* out, EnigmaticFeatures_p features);
void PrintEnigmaticBuckets(FILE* out, EnigmaticInfo_p info);

#endif

/*---------------------------------------------------------------------*/
/*                        End of File                                  */
/*---------------------------------------------------------------------*/

