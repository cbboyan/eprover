/*-----------------------------------------------------------------------

File  : cte_termtypes.h

Author: Stephan Schulz

Contents
 
  Declarations for the basic term type and primitive functions, mainly
  on single term cells. This module mostly provides only
  infrastructure for higher level modules.

  Copyright 1998, 1999 by the author.
  This code is released under the GNU General Public Licence and
  the GNU Lesser General Public License.
  See the file COPYING in the main E directory for details..
  Run "eprover -h" for contact information.

Changes

<1> Tue Feb 24 01:23:24 MET 1998
    Ripped out of the now obsolete cte_terms.h
<2> Thu Mar 28 21:40:52 CEST 2002
    Started to implement new rewriting

-----------------------------------------------------------------------*/

#ifndef CTE_TERMTYPES

#define CTE_TERMTYPES

#include <clb_partial_orderings.h>
#include <cte_signature.h>
#include <clb_sysdate.h>
#include <clb_ptrees.h>
#include <clb_properties.h>



/*---------------------------------------------------------------------*/
/*                    Data type declarations                           */
/*---------------------------------------------------------------------*/

#define DEFAULT_VWEIGHT  1  /* This has to be an integer > 0! */
#define DEFAULT_FWEIGHT  2  /* This has to be >= DEFAULT_VWEIGHT */

/* POWNRS = Probably obsolete with new rewriting scheme */

typedef enum 
{
   TPIgnoreProps      =      0, /* For masking properties out */
   TPRestricted       =      1, /* Rewriting is restricted on this term */
   TPTopPos           =      2, /* This cell is a entry point */
   TPIsGround         =      4, /* Shared term is ground */
   TPPredPos          =      8, /* This is an original predicate
                                   position morphed into a term */
   TPIsRewritable     =     16, /* Term is known to be rewritable with
                                   respect to a current rule or rule
                                   set. Used for removing
                                   backward-rewritable clauses. Absence of
                                   this flag does not mean that the term
                                   is in any kind of normal form! POWNRS */
   TPIsRRewritable    =     32, /* Term is rewritable even if
                                   rewriting is restricted to proper
                                   instances at the top level.*/
   TPIsSOSRewritten   =     64, /* Term has been rewritten with a SoS
                                   clause (at top level) */
   TPSpecialFlag      =    128, /* For internal use with normalizing variables*/
   TPOpFlag           =    256, /* For internal use */
   TPCheckFlag        =    512, /* For internal use */
   TPOutputFlag       =   1024, /* Has this term already been printed (and
                                   thus defined)? */
   TPIsSpecialVar     =   2048, /* Is this a meta-variable generated by term
                                   top operations and the like? */
   TPIsRewritten      =   4096, /* Term has been rewritten (for the new
                                   rewriting scheme) */ 
   TPIsRRewritten     =   8192, /* Term has been rewritten at a
                                   subterm position or with a real
                                   instance (for the new rewriting
                                   scheme) */ 
   TPIsShared         =  16384, /* Term is in a term bank */
   TPGarbageFlag      =  32768, /* For the term bank garbage collection */
   TPIsFreeVar        =  65536, /* For Skolemization */
   TPPotentialParamod = 131072, /* This position needs to be tried for
                                   paramodulation */
   TPPosPolarity      = 1<<18,  /* In the term encoding of a formula,
                                   this occurs with positive polarity. */
   TPNegPolarity      = 1<<19,  /* In the term encoding of a formula,
                                   this occurs with negative polarity. */
}TermProperties;



typedef enum  /* See CLAUSES/ccl_rewrite.c for more */
{
   NoRewrite = 0,     /* Just for completness */
   RuleRewrite = 1,   /* Rewrite with rules only */
   FullRewrite = 2    /* Rewrite with rules and equations */
}RewriteLevel;

typedef struct
{
   SysDate          nf_date[FullRewrite]; /* If term is not rewritten,
                                             it is in normal form with
                                             respect to the
                                             demodulators at this date */
   struct {
      struct termcell*   replace;         /* ...otherwise, it has been
                                             rewritten to this term */
      // long               demod_id;        /* 0 means subterm! */
      struct clause_cell *demod;          /* NULL means subterm! */
   }rw_desc;
}RewriteState;


typedef struct termcell
{
   FunCode          f_code;        /* Top symbol of term */
   TermProperties   properties;    /* Like basic, lhs, top */
   int              arity;         /* Redundant, but saves handing
                                      around the signature all the
                                      time */
   struct termcell* *args;         /* Pointer to array of arguments */
   struct termcell* binding;       /* For variable bindings,
                                      potentially for temporary
                           a           rewrites - it might be possible
                                      to combine the previous two in a
                                      union. */
   unsigned long    entry_no;      /* Counter for terms in a given
                                      termbank - needed for
                                      administration and external
                                      representation */
   long             weight;        /* Weight of the term, if term is
                                      in term bank */
   RewriteState     rw_data;       /* See above */
   struct termcell* lson;          /* For storing shared term nodes in */
   struct termcell* rson;          /* a splay tree - see
                                      cte_termcellstore.[ch] */
}TermCell, *Term_p, **TermRef;


typedef long DerefType, *DerefType_p;

#define DEREF_ALWAYS -1
#define DEREF_NEVER   0
#define DEREF_ONCE    1


/* The following is an estimate for the memory taken up by a term cell
   with arguments (the argument array is not counted separately). */

#ifdef CONSTANT_MEM_ESTIMATE
#define TERMCELL_MEM 48
#define TERMARG_MEM  4
#define TERMP_MEM    4
#else
#define TERMCELL_MEM MEMSIZE(TermCell)
#define TERMARG_MEM  sizeof(void*)
#define TERMP_MEM    sizeof(Term_p)
#endif

#define TERMCELL_DYN_MEM (TERMCELL_MEM+4*TERMARG_MEM)


/*---------------------------------------------------------------------*/
/*                Exported Functions and Variables                     */
/*---------------------------------------------------------------------*/

/* Functions which take two terms and return a boolean, i.e. test for
   equality */

typedef bool (*TermEqualTestFun)(Term_p t1, Term_p t2);

#define TERMS_INITIAL_ARGS 10

#define RewriteAdr(level) (assert(level),(level)-1)
#define TermIsVar(t) ((t)->f_code < 0)
#define TermIsConst(t)(!TermIsVar(t) && ((t)->arity==0))

#define TermCellSetProp(term, prop) SetProp((term), (prop))
#define TermCellDelProp(term, prop) DelProp((term), (prop))
#define TermCellAssignProp(term, sel, prop) AssignProp((term),(sel),(prop))
/* Are _all_ properties in prop set in term? */
#define TermCellQueryProp(term, prop) QueryProp((term), (prop))

/* Are any properties in prop set in term? */
#define TermCellIsAnyPropSet(term, prop) IsAnyPropSet((term), (prop))

#define TermCellGiveProps(term, props) GiveProps((term),(props))
#define TermCellFlipProp(term, props) FlipProp((term),(props))

#define TermCellAlloc() (TermCell*)SizeMalloc(sizeof(TermCell))
#define TermCellFree(junk)         SizeFree(junk, sizeof(TermCell))
#define TermArgArrayAlloc(arity) ((Term_p*)SizeMalloc((arity)*sizeof(Term_p)))
#define TermArgArrayFree(junk, arity) SizeFree((junk),(arity)*sizeof(Term_p))

#define TermIsRewritten(term) TermCellQueryProp((term), TPIsRewritten)
#define TermIsRRewritten(term) TermCellQueryProp((term), TPIsRRewritten)
#define TermIsTopRewritten(term) (TermIsRewritten(term)&&TermRWDemodField(term))
#define TermIsShared(term)       TermCellQueryProp((term), TPIsShared)

#define TermNFDate(term,i) (TermIsRewritten(term)?\
                           SysDateCreationTime():(term)->rw_data.nf_date[i])

/* Absolutely get the value of the replace and demod fields */
#define TermRWReplaceField(term) ((term)->rw_data.rw_desc.replace)
#define TermRWDemodField(term)   ((term)->rw_data.rw_desc.demod)
#define REWRITE_AT_SUBTERM 0

/* Get the logical value of the replaced term / demodulator */
#define TermRWReplace(term) (TermIsRewritten(term)?TermRWTargetField(term):NULL)
#define TermRWDemod(term) (TermIsRewritten(term)?TermRWDemodField(term):NULL)

Term_p  TermDefaultCellAlloc(void);
Term_p  TermConstCellAlloc(FunCode symbol);
Term_p  TermTopAlloc(FunCode f_code, int arity); 
void    TermTopFree(Term_p junk); 
void    TermFree(Term_p junk);
Term_p  TermAllocNewSkolem(Sig_p sig, PStack_p variables, bool atom);

void    TermSetProp(Term_p term, DerefType deref, TermProperties prop);
bool    TermSearchProp(Term_p term, DerefType deref, TermProperties prop);
void    TermDelProp(Term_p term, DerefType deref, TermProperties prop);
void    TermVarSetProp(Term_p term, DerefType deref, TermProperties prop);
bool    TermVarSearchProp(Term_p term, DerefType deref, TermProperties prop);
void    TermVarDelProp(Term_p term, DerefType deref, TermProperties prop);

static __inline__ Term_p  TermDeref(Term_p term, DerefType_p deref);

static __inline__ Term_p* TermArgListCopy(Term_p source);
#ifndef __cplusplus
static __inline__ Term_p  TermTopCopy(Term_p source);
#endif

void    TermStackSetProps(PStack_p stack, TermProperties prop);
void    TermStackDelProps(PStack_p stack, TermProperties prop);



/*---------------------------------------------------------------------*/
/*                  Inline functions                                   */
/*---------------------------------------------------------------------*/


/*-----------------------------------------------------------------------
//
// Function: TermDeref()
//
//   Dereference a term. deref* tells us how many derefences to do
//   at most, it will be decremented for each dereferenciation.
//
// Global Variables: -
//
// Side Effects    : -
//
/----------------------------------------------------------------------*/

static __inline__ Term_p TermDeref(Term_p term, DerefType_p deref)
{
   assert(TermIsVar(term)||!(term->binding));

   if(*deref == DEREF_ALWAYS)
   {
      while(term->binding)
      {
         term = term->binding;
      }
   }
   else
   {
      while(*deref)
      {
         if(!term->binding)
         {
            break;
         }
         term = term->binding;
         (*deref)--;     
      }
   }
   return term;
}


/*-----------------------------------------------------------------------
//
// Function: TermArgListCopy()
//
//   Return a copy of the argument array of source.
//
// Global Variables: -
//
// Side Effects    : Memory operations
//
/----------------------------------------------------------------------*/

static __inline__ Term_p* TermArgListCopy(Term_p source)
{
   Term_p *handle;
   int i;
   
   if(source->arity)
   {
      handle = TermArgArrayAlloc(source->arity);
      for(i=0; i<source->arity; i++)
      {
         handle[i] = source->args[i];
      }
   }
   else
   {
      handle = NULL;
   }
   return handle;
}

#ifndef __cplusplus

/* This function is only needed in the core E libraries (but in many
 * of those), and not in the C++ code of (some) programs that link to
 * E. It contains C++-unfriendly code, so it's just ignored in this
 * case. */

/*-----------------------------------------------------------------------
//
// Function: TermTopCopy()
//
//   Return a copy of the term node (and potential argument
//   pointers). Only the top node and the pointers are duplicated, the
//   arguments are shared between source and copy. As this function
//   operates on nodes, it does not follow bindings! Administrative
//   stuff (refs etc. will, of course, not be copied but initialized
//   to rational values for an unshared 
//   term).
//
// Global Variables: -
//
// Side Effects    : Memory operations
//
/----------------------------------------------------------------------*/

static __inline__ Term_p TermTopCopy(Term_p source)
{
   Term_p handle;
   
   handle = TermDefaultCellAlloc();
   handle->properties = (source->properties&TPPredPos); /* All other
                                                           properties
                                                           are tied to
                                                           the specific
                                                           term! */
   TermCellDelProp(handle, TPOutputFlag); /* As it gets a new id below */
   handle->f_code = source->f_code;
   handle->arity  = source->arity;
   handle->binding = NULL;
   handle->args = TermArgListCopy(source);
   handle->lson = NULL;
   handle->rson = NULL;
   
   return handle;
}

#endif

#endif

/*---------------------------------------------------------------------*/
/*                        End of File                                  */
/*---------------------------------------------------------------------*/



