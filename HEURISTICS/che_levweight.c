/*-----------------------------------------------------------------------

File  : che_levweight.c

Author: could be anyone

Contents
 
  Auto generated. Your comment goes here ;-).

  Copyright 2016 by the author.
  This code is released under the GNU General Public Licence.
  See the file COPYING in the main directory for details.
  Run "eprover -h" for contact information.

Changes

<1> Tue Mar  8 22:40:31 CET 2016
    New

-----------------------------------------------------------------------*/

#include <float.h>
#include "che_levweight.h"

/*---------------------------------------------------------------------*/
/*                        Global Variables                             */
/*---------------------------------------------------------------------*/

/*---------------------------------------------------------------------*/
/*                      Forward Declarations                           */
/*---------------------------------------------------------------------*/

/*---------------------------------------------------------------------*/
/*                         Internal Functions                          */
/*---------------------------------------------------------------------*/

static PStack_p lev_compute_term_code(Term_p term)
{
   PStack_p stack;
   PStack_p code;
   Term_p sub_term;

   code = PStackAlloc();
   stack = PStackAlloc();
   
   TermLRTraverseInit(stack,term);
   while ((sub_term = TermLRTraverseNext(stack))) {
      PStackPushInt(code,sub_term->f_code);
   }
   PStackFree(stack);

   return code;
}

static void lev_insert_term(
   PStack_p codes,
   Term_p term, 
   VarBank_p vars, 
   VarNormStyle var_norm)
{
   Term_p norm;
   PStack_p code;
  
   norm = TermCopyNormalizeVars(vars,term,var_norm);
   code = lev_compute_term_code(norm);
   PStackPushP(codes,code);

   TermFree(norm);
}

static void lev_insert_subterms(
   PStack_p codes,
   Term_p term, 
   VarBank_p vars, 
   VarNormStyle var_norm)
{
   int i;
   PStack_p stack;
   Term_p subterm;

   stack = PStackAlloc();

   PStackPushP(stack, term);
   while (!PStackEmpty(stack))
   {
      subterm = PStackPopP(stack);
      if(TermIsVar(subterm)) {
         continue;
      }
      lev_insert_term(codes,subterm,vars,var_norm);

      for(i=0; i<subterm->arity; i++)
      {
         PStackPushP(stack, subterm->args[i]);
      }
   }

   PStackFree(stack);
}

static void lev_insert_topgens(
   PStack_p codes, 
   Term_p term, 
   VarBank_p vars,
   Sig_p sig,
   VarNormStyle var_norm)
{
   int i;
   PStack_p topgens;
   Term_p topgen;

   topgens = ComputeTopGeneralizations(term,vars,sig);
   for (i=0; i<topgens->current; i++) {
      topgen = topgens->stack[i].p_val;
      lev_insert_term(codes,topgen,vars,var_norm);
   }
   FreeGeneralizations(topgens);
}

static void lev_insert_subgens(
   PStack_p codes, 
   Term_p term, 
   VarBank_p vars, 
   VarNormStyle var_norm)
{
   int i;
   PStack_p subgens;
   Term_p genterm;

   subgens = ComputeSubtermsGeneralizations(term,vars);
   for (i=0; i<subgens->current; i++) {
      genterm = subgens->stack[i].p_val;
      lev_insert_term(codes,genterm,vars,var_norm);
   }
   FreeGeneralizations(subgens);
}

static void lev_init(LevWeightParam_p data)
{
   Clause_p clause;
   Clause_p anchor;
   Eqn_p lit;

   if (data->codes) {
      return;
   }

   data->codes = PStackAlloc();
   data->vars = VarBankAlloc(SortTableAlloc());
   
   // for each axiom ...
   anchor = data->proofstate->axioms->anchor;
   for (clause=anchor->succ; clause!=anchor; clause=clause->succ)
   {
      if(ClauseQueryTPTPType(clause)!=CPTypeNegConjecture) {
         continue;
      }
      
      // for each literal of a negated conjecture ...
      for (lit=clause->literals; lit; lit=lit->next)
      {
         switch (data->rel_terms) {
         case RTSConjectureTerms:
            lev_insert_term(
               data->codes,lit->lterm,data->vars,data->var_norm);
            lev_insert_term(
               data->codes,lit->rterm,data->vars,data->var_norm);
            break;
         case RTSConjectureSubterms:
            lev_insert_subterms(
               data->codes,lit->lterm,data->vars,data->var_norm);
            lev_insert_subterms(
               data->codes,lit->rterm,data->vars,data->var_norm);
            break;
         case RTSConjectureSubtermsTopGens:
            lev_insert_subterms(
               data->codes,lit->lterm,data->vars,data->var_norm);
            lev_insert_subterms(
               data->codes,lit->rterm,data->vars,data->var_norm);
            lev_insert_topgens(data->codes,lit->lterm,data->vars,
               data->ocb->sig,data->var_norm);
            lev_insert_topgens(data->codes,lit->rterm,data->vars,
               data->ocb->sig,data->var_norm);
            break;
         case RTSConjectureSubtermsAllGens:
            lev_insert_subgens(
               data->codes,lit->lterm,data->vars,data->var_norm);
            lev_insert_subgens(
               data->codes,lit->rterm,data->vars,data->var_norm);
            break;
         default:
            Error("ConjectureLevDistanceWeight parameters usage error (unsupported RelatedTermSet %d)", USAGE_ERROR, data->rel_terms);
            break;
         }
      }
   }
}

static double lev_codes_distance(
   PStack_p code1,
   PStack_p code2,
   int ins_cost, 
   int del_cost, 
   int ch_cost)
{
   unsigned int x, y, lastdiag, olddiag;
   unsigned int del, ins, ch;
   FunCode* s1 = (FunCode*)code1->stack;
   FunCode* s2 = (FunCode*)code2->stack;
   unsigned int s1len = code1->current;
   unsigned int s2len = code2->current;
   unsigned int column[s1len+1];

   for (y=0; y<=s1len; y++) 
   {
      column[y] = y*del_cost;
   }
   for (x=1;x<=s2len; x++) 
   {
      column[0] = x*ins_cost;;
      for (y=1,lastdiag=(x-1)*ins_cost; y<=s1len; y++) 
      {
         olddiag = column[y];
         del = column[y]+del_cost;
         ins = column[y-1]+ins_cost;
         ch = lastdiag+(s1[y-1]==s2[x-1] ? 0 : ch_cost);
         column[y] = MIN3(del,ins,ch);
         lastdiag = olddiag;
      }
   }

   return(column[s1len]);
}

static double lev_term_weight(Term_p term, LevWeightParam_p data)
{
   int i;
   double min;
   PStack_p norm_code;
   PStack_p conj_code;
   Term_p norm;

   norm = TermCopyNormalizeVars(data->vars,term,data->var_norm);
   norm_code = lev_compute_term_code(norm);

   min = DBL_MAX;
   for (i=0; i<data->codes->current; i++) 
   {
      conj_code = data->codes->stack[i].p_val;
      min = MIN(min,lev_codes_distance(norm_code,conj_code,
         data->ins_cost,data->del_cost,data->ch_cost));
   }

   PStackFree(norm_code);
   TermFree(norm);

   return min;
}

/*---------------------------------------------------------------------*/
/*                         Exported Functions                          */
/*---------------------------------------------------------------------*/

LevWeightParam_p LevWeightParamAlloc(void)
{
   LevWeightParam_p res = LevWeightParamCellAlloc();
   
   res->codes = NULL;
   res->vars  = NULL;
   
   return res;
}

void LevWeightParamFree(LevWeightParam_p junk)
{
   PStack_p code;

   if (junk->codes) 
   {
      while (!PStackEmpty(junk->codes)) {
         code = PStackPopP(junk->codes);
         PStackFree(code);
      }
      PStackFree(junk->codes);
      junk->codes = NULL;
   }
   if (junk->vars)
   {
      VarBankFree(junk->vars);
      junk->vars = NULL;
   }
   LevWeightParamCellFree(junk);
}
 
WFCB_p ConjectureLevDistanceWeightParse(
   Scanner_p in,  
   OCB_p ocb, 
   ProofState_p state)
{   
   ClausePrioFun prio_fun;
   double pos_multiplier, max_term_multiplier, max_literal_multiplier;
   TermWeightExtenstionStyle ext_style;
   VarNormStyle var_norm;
   RelatedTermSet rel_terms;
   int ins_cost, del_cost, ch_cost;

   AcceptInpTok(in, OpenBracket);
   prio_fun = ParsePrioFun(in);
   AcceptInpTok(in, Comma);

   var_norm = (VarNormStyle)ParseInt(in);
   AcceptInpTok(in, Comma);
   rel_terms = (RelatedTermSet)ParseInt(in);
   AcceptInpTok(in, Comma);

   ins_cost = ParseInt(in);
   AcceptInpTok(in, Comma);
   del_cost = ParseInt(in);
   AcceptInpTok(in, Comma);
   ch_cost = ParseInt(in);
   AcceptInpTok(in, Comma);

   ext_style = (TermWeightExtenstionStyle)ParseInt(in);
   AcceptInpTok(in, Comma);
   max_term_multiplier = ParseFloat(in);
   AcceptInpTok(in, Comma);
   max_literal_multiplier = ParseFloat(in);
   AcceptInpTok(in, Comma);
   pos_multiplier = ParseFloat(in);
   AcceptInpTok(in, CloseBracket);
   
   return ConjectureLevDistanceWeightInit(
      prio_fun, 
      ocb,
      state,
      var_norm,
      rel_terms,
      ins_cost,
      del_cost,
      ch_cost,
      ext_style,
      max_term_multiplier,
      max_literal_multiplier,
      pos_multiplier);
}

WFCB_p ConjectureLevDistanceWeightInit(
   ClausePrioFun prio_fun, 
   OCB_p ocb,
   ProofState_p proofstate,
   VarNormStyle var_norm,
   RelatedTermSet rel_terms,
   int ins_cost,
   int del_cost, 
   int ch_cost,
   TermWeightExtenstionStyle ext_style,
   double max_term_multiplier,
   double max_literal_multiplier,
   double pos_multiplier)
{
   LevWeightParam_p data = LevWeightParamAlloc();

   data->init_fun   = lev_init;
   data->ocb        = ocb;
   data->proofstate = proofstate;
   data->var_norm   = var_norm;
   data->rel_terms  = rel_terms;
   data->ins_cost   = ins_cost;
   data->del_cost   = del_cost;
   data->ch_cost    = ch_cost;
   data->twe = TermWeightExtensionAlloc(
      max_term_multiplier,
      max_literal_multiplier,
      pos_multiplier,
      ext_style,
      (TermWeightFun)lev_term_weight,
      data);
   
   return WFCBAlloc(
      ConjectureLevDistanceWeightCompute, 
      prio_fun,
      ConjectureLevDistanceWeightExit, 
      data);
}

double ConjectureLevDistanceWeightCompute(void* data, Clause_p clause)
{
   double res;
   LevWeightParam_p local;
   
   local = data;
   local->init_fun(data);

   ClauseCondMarkMaximalTerms(local->ocb, clause);
   res = ClauseTermExtWeight(clause, local->twe);

#ifdef DEBUG_TERMWEIGHTS
   fprintf(GlobalOut, "=%.2f: ", res);
   ClausePrint(GlobalOut, clause, true);
   fprintf(GlobalOut, "\n");
#endif

   return res;
}

void ConjectureLevDistanceWeightExit(void* data)
{
   LevWeightParam_p junk = data;
   
   LevWeightParamFree(junk);
}

/*---------------------------------------------------------------------*/
/*                        End of File                                  */
/*---------------------------------------------------------------------*/

