/*-----------------------------------------------------------------------

File  : che_treeweight.c

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
#include "che_treeweight.h"


/*---------------------------------------------------------------------*/
/*                        Global Variables                             */
/*---------------------------------------------------------------------*/

//static Sig_p SIG;

/*---------------------------------------------------------------------*/
/*                      Forward Declarations                           */
/*---------------------------------------------------------------------*/

/*---------------------------------------------------------------------*/
/*                         Internal Functions                          */
/*---------------------------------------------------------------------*/

static void ted_insert_term(
   PStack_p terms,
   Term_p term, 
   VarBank_p vars, 
   VarNormStyle var_norm)
{
   Term_p norm;
   
   norm = TermCopyNormalizeVars(vars,term,var_norm);
   PStackPushP(terms,norm);
}

static void ted_insert_subterms(
   PStack_p terms,
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
      ted_insert_term(terms,subterm,vars,var_norm);

      for(i=0; i<subterm->arity; i++)
      {
         PStackPushP(stack, subterm->args[i]);
      }
   }

   PStackFree(stack);
}

static void ted_insert_topgens(
   PStack_p terms, 
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
      ted_insert_term(terms,topgen,vars,var_norm);
   }
   FreeGeneralizations(topgens);
}

static void ted_insert_subgens(
   PStack_p terms, 
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
      ted_insert_term(terms,genterm,vars,var_norm);
   }
   FreeGeneralizations(subgens);
}

static void ted_init(TreeWeightParam_p data)
{
   Clause_p clause;
   Clause_p anchor;
   Eqn_p lit;

   if (data->terms) {
      return;
   }

   //SIG = data->ocb->sig;

   data->terms = PStackAlloc();
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
            ted_insert_term(
               data->terms,lit->lterm,data->vars,data->var_norm);
            ted_insert_term(
               data->terms,lit->rterm,data->vars,data->var_norm);
            break;
         case RTSConjectureSubterms:
            ted_insert_subterms(
               data->terms,lit->lterm,data->vars,data->var_norm);
            ted_insert_subterms(
               data->terms,lit->rterm,data->vars,data->var_norm);
            break;
         case RTSConjectureSubtermsTopGens:
            ted_insert_subterms(
               data->terms,lit->lterm,data->vars,data->var_norm);
            ted_insert_subterms(
               data->terms,lit->rterm,data->vars,data->var_norm);
            ted_insert_topgens(data->terms,lit->lterm,data->vars,
               data->ocb->sig,data->var_norm);
            ted_insert_topgens(data->terms,lit->rterm,data->vars,
               data->ocb->sig,data->var_norm);
            break;
         case RTSConjectureSubtermsAllGens:
            ted_insert_subgens(
               data->terms,lit->lterm,data->vars,data->var_norm);
            ted_insert_subgens(
               data->terms,lit->rterm,data->vars,data->var_norm);
            break;
         default:
            Error("ConjectureTreeDistanceWeight parameters usage error (unsupported RelatedTermSet %d)", USAGE_ERROR, data->rel_terms);
            break;
         }
      }
   }
}


static void ted_lmld_kr(
   Term_p term, 
   long* l,
   PStack_p kr,
   FunCode* code,
   long* fresh, 
   bool isroot)
{
   int i;

   if (TermIsVar(term)||TermIsConst(term)) 
   {
      term->freq = (*fresh)++;
      l[term->freq] = term->freq;
   }
   else 
   {
      for (i=0; i<term->arity; i++)
      {
         ted_lmld_kr(term->args[i],l,kr,code,fresh,(i!=0));
      }

      term->freq = (*fresh)++;
      l[term->freq] = l[term->args[0]->freq];
   }

   code[term->freq] = term->f_code;
   if (isroot) 
   {
      PStackPushInt(kr,term->freq);
      //printf(">>> key-root %ld\n", term->freq);
   }

   //printf(">>> id=%ld; left-leaf=%ld     ",term->freq,l[term->freq]);
   //TermPrint(GlobalOut,term,SIG,DEREF_NEVER);
   //printf("\n");
}

static void ted_forest_distance(
   long i, 
   long j, 
   long* l1, 
   long* l2, 
   FunCode* code1,
   FunCode* code2,
   int len2,
   long td[][len2],
   int ins_cost, 
   int del_cost, 
   int ch_cost)
{
   long fd[i+1][j+1];
   long di, dj;
   
   for (di=0; di<=i; di++) 
   {
      for (dj=0; dj<=j; dj++) 
      {
         fd[di][dj] = 0L;
      }
   }

   fd[l1[i]-1][l2[j]-1] = 0;
   for (di=l1[i]; di<=i; di++) 
   {
      fd[di][l2[j]-1] = fd[di-1][l2[j]-1]+del_cost;
   }
   for (dj=l2[j]; dj<=j; dj++) 
   {
      fd[l1[i]-1][dj] = fd[l1[i]-1][dj-1]+ins_cost;
   }
   for (di=l1[i]; di<=i; di++)
   {
      for (dj=l2[j]; dj<=j; dj++)
      {
         if ((l1[di]==l1[i]) && (l2[dj]==l2[j]))
         {
            fd[di][dj] = MIN3(
               fd[di-1][dj]+del_cost,
               fd[di][dj-1]+ins_cost,
               fd[di-1][dj-1]+(code1[di]==code2[dj]?0:ch_cost));
            td[di][dj] = fd[di][dj];
         }
         else
         {
            fd[di][dj] = MIN3(
               fd[di-1][dj]+del_cost,
               fd[di][dj-1]+ins_cost,
               fd[l1[di]-1][l2[dj]-1]+td[di][dj]);
         }
      }
   }
}

static double ted_term_distance(
   Term_p term1,
   Term_p term2,
   int ins_cost, 
   int del_cost, 
   int ch_cost)
{
   long len1 = TermWeight(term1,1,1);
   long len2 = TermWeight(term2,1,1);
   long l1[len1+1];
   long l2[len2+1];
   FunCode code1[len1+1];
   FunCode code2[len2+1];
   long td[len1+1][len2+1];
   PStack_p kr1, kr2;
   long fresh;
   long x, y;

   for (x=0; x<=len1; x++) 
   {
      for (y=0; y<=len2; y++) 
      {
         td[x][y] = 0L;
      }
   }
   
   kr1 = PStackAlloc();
   kr2 = PStackAlloc();
   
   fresh=1; ted_lmld_kr(term1,l1,kr1,code1,&fresh,true);
   fresh=1; ted_lmld_kr(term2,l2,kr2,code2,&fresh,true);

   for (x=0; x<kr1->current; x++) 
   {
      for (y=0; y<kr2->current; y++)
      {
         ted_forest_distance( 
            kr1->stack[x].i_val, kr2->stack[y].i_val,
            l1, l2, code1, code2, len2+1, td,
            ins_cost, del_cost, ch_cost);      
      }
   }

   PStackFree(kr1);
   PStackFree(kr2);
   
   //printf("TED-DISTANCE    ");
   //TermPrint(GlobalOut,term1,SIG,DEREF_NEVER);
   //printf(" &&  ");
   //TermPrint(GlobalOut,term2,SIG,DEREF_NEVER);
   //printf(" = %ld\n",td[len1][len2]);

   return td[len1][len2];
}

static double ted_term_weight(Term_p term, TreeWeightParam_p data)
{
   int i;
   double min;
   Term_p conj, norm;

   norm = TermCopyNormalizeVars(data->vars,term,data->var_norm);

   min = DBL_MAX;
   for (i=0; i<data->terms->current; i++) 
   {
      conj = data->terms->stack[i].p_val;
      min = MIN(min,ted_term_distance(norm,conj,
         data->ins_cost,data->del_cost,data->ch_cost));
   }

   TermFree(norm);

   return min;
}

/*---------------------------------------------------------------------*/
/*                         Exported Functions                          */
/*---------------------------------------------------------------------*/

TreeWeightParam_p TreeWeightParamAlloc(void)
{
   TreeWeightParam_p res = TreeWeightParamCellAlloc();
   
   res->terms = NULL;
   res->vars  = NULL;
   
   return res;
}

void TreeWeightParamFree(TreeWeightParam_p junk)
{
   Term_p term;

   if (junk->terms) 
   {
      while (!PStackEmpty(junk->terms)) {
         term = PStackPopP(junk->terms);
         TermFree(term);
      }
      PStackFree(junk->terms);
      junk->terms = NULL;
   }
   if (junk->vars)
   {
      VarBankFree(junk->vars);
      junk->vars = NULL;
   }
   TreeWeightParamCellFree(junk);
}
 
WFCB_p ConjectureTreeDistanceWeightParse(
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
   
   return ConjectureTreeDistanceWeightInit(
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

WFCB_p ConjectureTreeDistanceWeightInit(
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
   TreeWeightParam_p data = TreeWeightParamAlloc();

   data->init_fun   = ted_init;
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
      (TermWeightFun)ted_term_weight,
      data);
   
   return WFCBAlloc(
      ConjectureTreeDistanceWeightCompute, 
      prio_fun,
      ConjectureTreeDistanceWeightExit, 
      data);
}

double ConjectureTreeDistanceWeightCompute(void* data, Clause_p clause)
{
   TreeWeightParam_p local;
   
   local = data;
   local->init_fun(data);

   ClauseCondMarkMaximalTerms(local->ocb, clause);
   return ClauseTermExtWeight(clause, local->twe);
}

void ConjectureTreeDistanceWeightExit(void* data)
{
   TreeWeightParam_p junk = data;
   
   TreeWeightParamFree(junk);
}

/*---------------------------------------------------------------------*/
/*                        End of File                                  */
/*---------------------------------------------------------------------*/

