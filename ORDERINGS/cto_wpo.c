/*-----------------------------------------------------------------------

File:    cto_wpo.c

Author:  Jan Jakubuv

Contents:
TODO

Copyright 1998, 1999,2004 by the author.
This code is released under the GNU General Public Licence and
the GNU Lesser General Public License.
See the file COPYING in the main E directory for details..
Run "eprover -h" for contact information.

Changes


-----------------------------------------------------------------------*/

#include "cto_wpo.h"

/*---------------------------------------------------------------------*/
/*                        Global Variables                             */
/*---------------------------------------------------------------------*/

#define MAX_INDEX 20
#define VAR_INDEX(f_code) (-(((f_code)-1)/2))
#define COMPARE(x,y) (((x)<(y))?to_lesser:((x)==(y)?to_equal:to_greater))

/*---------------------------------------------------------------------*/
/*                      Forward Declarations                           */
/*---------------------------------------------------------------------*/

/*---------------------------------------------------------------------*/
/*                         Internal Functions                          */
/*---------------------------------------------------------------------*/

static void wpo_algebra_sum_poly(OCB_p ocb, Term_p t, DerefType deref, long* poly, int dim)
{
   int i,j;
   long coef = 1;

   t = TermDeref(t, &deref);
   if (TermIsVar(t))
   {
      poly[VAR_INDEX(t->f_code)] = ocb->var_weight;
   }
   else
   {
      long tmp[MAX_INDEX];
      poly[0] = OCBFunWeight(ocb, t->f_code);
      for (i=0; i<t->arity; i++)
      {
         for (j=0; j<dim; j++) { tmp[j] = 0L; }

         wpo_algebra_sum_poly(ocb, t->args[i], deref, tmp, dim);

         coef = (long)*OCBAlgebraCoefPos(ocb,t->f_code,i);
         for (j=0; j<dim; j++) { poly[j] += (coef * tmp[j]); }
      }
   }
}

static void wpo_algebra_max_poly(OCB_p ocb, Term_p t, DerefType deref, long* poly, int dim)
{
   int i,j;
   long coef = 1;

   t = TermDeref(t, &deref);
   if (TermIsVar(t))
   {
      poly[VAR_INDEX(t->f_code)] = 0L;
   }
   else
   {
      long tmp[MAX_INDEX];
      poly[0] = OCBFunWeight(ocb, t->f_code);
      for (i=0; i<t->arity; i++)
      {
         for (j=0; j<dim; j++) { tmp[j] = -1; }

         wpo_algebra_max_poly(ocb, t->args[i], deref, tmp, dim);

         coef = (long)*OCBAlgebraCoefPos(ocb,t->f_code,i);
         for (j=0; j<dim; j++) { if (tmp[j] != -1) {poly[j] = MAX(poly[j], coef+tmp[j]);} }
      }
   }
}

CompareResult wpo_algebra_poly_compare(long* poly_s, long* poly_t, int dim)
{
   int i;
   int mis_s = 0;
   int mis_t = 0;
   CompareResult sofar = to_unknown;
   CompareResult current = to_unknown;

   // TODO: this might not work with Max when we allow w(f)=0
   sofar = COMPARE(poly_s[0], poly_t[0]);
   for (i=1; i<dim; i++)
   {
      if ((poly_s[i] == -1) && (poly_t[i] !=  -1))
      {
         mis_s++;
         continue;
      }
      if ((poly_t[i] == -1) && (poly_s[i] !=  -1))
      {
         mis_t++;
         continue;
      }

      current = COMPARE(poly_s[i], poly_t[i]);
      switch (current)
      {
         case to_lesser:
            if (sofar == to_greater)
            {
               return to_uncomparable;
            }
            sofar = to_lesser;
            break;
         case to_greater:
            if (sofar == to_lesser)
            {
               return to_uncomparable;
            }
            sofar = to_greater;
            break;
         default: // implies to_equal
            break;
      }
   }

   if ((mis_s > 0) && (mis_t > 0))
   {
      return to_uncomparable;
   }

   return sofar;
}

static CompareResult wpo_algebra_compare(
   OCB_p ocb, 
   Term_p s, 
   Term_p t, 
   DerefType deref_s, 
   DerefType deref_t,
   int dim)
{
   int i;
   long poly_s[MAX_INDEX];
   long poly_t[MAX_INDEX];
   CompareResult res;

   
   switch (ocb->algebra) 
   {
      case Sum: 
         for (i=0; i<dim; i++) { poly_s[i] = 0L; }
         for (i=0; i<dim; i++) { poly_t[i] = 0L; }
         wpo_algebra_sum_poly(ocb, s, deref_s, poly_s, dim);
         wpo_algebra_sum_poly(ocb, t, deref_t, poly_t, dim);
         res = wpo_algebra_poly_compare(poly_s, poly_t, dim);

         if (OutputLevel >= 2)
         {
            fprintf(GlobalOut, "<WPO POLY> ");
            TermPrint(GlobalOut, s, ocb->sig, deref_s);
            fprintf(GlobalOut, " := %ld", poly_s[0]);
            for (i=1; i<dim; i++) 
            { 
               fprintf(GlobalOut, " + %ld*X%d", poly_s[i], i);
            }
            fprintf(GlobalOut, "\n<WPO POLY> ");
            TermPrint(GlobalOut, t, ocb->sig, deref_t);
            fprintf(GlobalOut, " := %ld", poly_t[0]);
            for (i=1; i<dim; i++) 
            { 
               fprintf(GlobalOut, " + %ld*X%d", poly_t[i], i);
            }
            fprintf(GlobalOut, "\n<WPO POLY> comparison :: %s ::\n", POCompareSymbol[res]);
         }
         return res;

      case Max:
         for (i=0; i<dim; i++) { poly_s[i] = -1; }
         for (i=0; i<dim; i++) { poly_t[i] = -1; }
         wpo_algebra_max_poly(ocb, s, deref_s, poly_s, dim);
         wpo_algebra_max_poly(ocb, t, deref_t, poly_t, dim);
         res = wpo_algebra_poly_compare(poly_s, poly_t, dim);
         
         if (OutputLevel >= 2)
         {
            fprintf(GlobalOut, "<WPO POLY> ");
            TermPrint(GlobalOut, s, ocb->sig, deref_s);
            fprintf(GlobalOut, " := max( %ld", poly_s[0]);
            for (i=1; i<dim; i++) 
            { 
               if (poly_s[i] == -1) { continue; }
               fprintf(GlobalOut, " , %ld+X%d", poly_s[i], i);
            }
            fprintf(GlobalOut, " )\n<WPO POLY> ");
            TermPrint(GlobalOut, t, ocb->sig, deref_t);
            fprintf(GlobalOut, " := max( %ld", poly_t[0]);
            for (i=1; i<dim; i++) 
            { 
               if (poly_t[i] == -1) { continue; }
               fprintf(GlobalOut, " , %ld+X%d", poly_t[i], i);
            }
            fprintf(GlobalOut, " )\n<WPO POLY> comparison :: %s ::\n", POCompareSymbol[res]);
         }
         return res;

      default:
         Error("WPO: Unrecognized value for ordering algebra (%d)", OTHER_ERROR, 
            ocb->algebra);
         return 0; // silent a compiler warning
   }

   return to_uncomparable;
}

static CompareResult wpo_args_lex_cmp(OCB_p ocb, Term_p s, Term_p t,
      DerefType deref_s, DerefType deref_t)
{
   int i;
   CompareResult cmp;
   long min;

   //s = TermDeref(s, &deref_s); // already derefed
   //t = TermDeref(t, &deref_t);
   
   min = MIN(s->arity, t->arity);
   for (i=0; i<min; i++) 
   {
      cmp = WPOCompare(ocb, s->args[i], t->args[i], deref_s, deref_t);
      if (cmp != to_equal) 
      {
         return cmp;
      }
   }

   if (s->arity == t->arity)
   {
      return to_equal;
   }
   else if (s->arity < t->arity)
   {
      return to_lesser;
   }
   else
   {
      return to_greater;
   }
}



static void wpo_vars_collect(Term_p lhs, NumTree_p* vars)
{
   IntOrP dummy;

   if (TermIsVar(lhs))
   {
      NumTreeStore(vars, lhs->f_code, dummy, dummy);
      return;
   }

   for (int i=0; i<lhs->arity; i++)
   {
      wpo_vars_collect(lhs->args[i], vars); 
   }
}

static bool wpo_vars_contained(Term_p rhs, NumTree_p* vars)
{
   if (TermIsVar(rhs))
   {
      return (NumTreeFind(vars, rhs->f_code) != NULL);
   }

   for (int i=0; i<rhs->arity; i++)
   {
      if (!wpo_vars_contained(rhs->args[i], vars))
      {
         return false;
      }
   }
   return true;
}

static bool wpo_fresh_check(Term_p lhs, Term_p rhs)
{
   NumTree_p vars = NULL;
   wpo_vars_collect(lhs, &vars);
   bool ret = wpo_vars_contained(rhs, &vars);
   NumTreeFree(vars);
   return ret;
}

/*---------------------------------------------------------------------*/
/*                      Exported Functions                             */
/*---------------------------------------------------------------------*/

/*-----------------------------------------------------------------------
//
// Function: WPOCompare(ocb, s, t)
//
// Global Variables: -
//
// Side Effects    : -
//
-----------------------------------------------------------------------*/

CompareResult WPOCompare(OCB_p ocb, Term_p s, Term_p t,
      DerefType deref_s, DerefType deref_t)
{
   int i, dim;
   //long sweight, tweight;
   bool all_greater, all_lesser;
   CompareResult algebra_cmp = to_unknown;
   CompareResult top_sym_cmp = to_unknown;
   CompareResult args_lex_cmp = to_unknown;
       
   s = TermDeref(s, &deref_s);
   t = TermDeref(t, &deref_t);

   FunCode max_s = TermFindMaxVarCode(s);
   FunCode max_t = TermFindMaxVarCode(t);
   dim = 1+MAX(VAR_INDEX(max_s), VAR_INDEX(max_t));
   if (dim > MAX_INDEX)
   {
      return to_uncomparable;
   }

   algebra_cmp = wpo_algebra_compare(ocb, s, t, deref_s, deref_t, dim);
   switch (algebra_cmp) 
   {
      case to_lesser:
         return wpo_fresh_check(t,s) ? to_lesser : to_uncomparable;
      case to_greater:
         return wpo_fresh_check(s,t) ? to_greater : to_uncomparable;
      case to_uncomparable:
         return to_uncomparable;
      default: // implies to_equal
         break;
   }

   // only the same terms are =WPO
   if (TermStructEqualDeref(s, t, deref_s, deref_t))
   {
      return to_equal;
   }
   //if (TermStructEqualDerefNoVars(s, t, deref_s, deref_t))
   //{
   //   return to_uncomparable;
   //}
   
   // is there argument s_i such that s_i >=WPO t ?
   if (s->arity > 0) 
   {
      all_lesser = true;
      for (i=0; i<s->arity; i++)
      {
         switch (WPOCompare(ocb, s->args[i], t, deref_s, deref_t))
         {
            case to_greater:
            case to_equal:
               //return to_greater;
               return wpo_fresh_check(s,t) ? to_greater : to_uncomparable;
            case to_lesser:
               break; // just keep the value all_lesser = true
            default:
               all_lesser = false;
         }
      }
      // are all arguments s_i <WPO t ?
      if ((t->arity > 0) && all_lesser)
      {
         // if so, compare head symbols
         top_sym_cmp = OCBFunCompare(ocb, s->f_code, t->f_code);
         switch (top_sym_cmp)
         {
            case to_lesser:
               //return to_lesser;
               return wpo_fresh_check(t,s) ? to_lesser : to_uncomparable;
            case to_equal:
               // if heads are equal, compare arguments lexicographically
               // (and remember the result for it might be used later)
               args_lex_cmp = wpo_args_lex_cmp(ocb, s, t, deref_s, deref_t);
               if (args_lex_cmp == to_lesser) 
               {
                  //return to_lesser;
                  return wpo_fresh_check(t,s) ? to_lesser : to_uncomparable;
               }
            default:
               break;
         }
      }
   }
  
   // a mirror case of the above
   if (t->arity > 0)
   {
      all_greater = true;
      for (i=0; i<t->arity; i++)
      {
         switch (WPOCompare(ocb, s, t->args[i], deref_s, deref_t))
         {
            case to_lesser:
            case to_equal:
               //return to_lesser;
               return wpo_fresh_check(t,s) ? to_lesser : to_uncomparable;
            case to_greater:
               break;
            default:
               all_greater = false;
         }
      }
      if ((s->arity > 0) && all_greater)
      {
         if (top_sym_cmp == to_unknown) {
            top_sym_cmp = OCBFunCompare(ocb, s->f_code, t->f_code);
         }
         switch (top_sym_cmp)
         {
            case to_greater:
               //return to_greater;
               return wpo_fresh_check(s,t) ? to_greater : to_uncomparable;
            case to_equal:
               if (args_lex_cmp == to_unknown) 
               {
                  args_lex_cmp = wpo_args_lex_cmp(ocb, s, t, deref_s, deref_t);
               }
               if (args_lex_cmp == to_greater)
               {
                  //return to_greater;
                  return wpo_fresh_check(s,t) ? to_greater : to_uncomparable;
               }
            default:
               break;
         }
      }
   }

   return to_uncomparable;
}

/*-----------------------------------------------------------------------
//
// Function: WPOGreater(ocb, s, t)
//
// Global Variables: -
//
// Side Effects    : -
//
-----------------------------------------------------------------------*/

bool WPOGreater(OCB_p ocb, Term_p s, Term_p t,
      DerefType deref_s, DerefType deref_t)
{
   return WPOCompare(ocb,s,t,deref_s,deref_t) == to_greater;
}

/*---------------------------------------------------------------------*/
/*                        End of File                                  */
/*---------------------------------------------------------------------*/

