#------------------------------------------------------------------------
#
# File  : Makefile for simple applications build on top of (parts of)
#         CLIB. 
#
# Author: Stephan Schulz
#
# Changes
#
# <1> Fri Nov 28 00:25:20 MET 1997
#     New
#
#------------------------------------------------------------------------

include ../Makefile.vars

# Project specific variables

PROJECT = ex_commandline term2dag tptp-sig
LIB     = $(PROJECT)
all: $(LIB)

depend: *.c
	$(MAKEDEPEND)

# Remove all automatically generated files

clean:
	@touch does_exist.o does_exist.dSYM does_exist.a $(PROJECT)
	@rm -r *.o $(PROJECT) *.dSYM *.a

# Services (provided by the master Makefile)

include ../Makefile.services

# Build the test programs

TERM2DAG = term2dag.o ../lib/TERMS.a ../lib/INOUT.a ../lib/BASICS.a

term2dag: $(TERM2DAG)
	$(LD) -o term2dag $(TERM2DAG) $(LIBS)

EX_COMMANDLINE = ex_commandline.o ../lib/INOUT.a ../lib/BASICS.a

ex_commandline: $(EX_COMMANDLINE)
	$(LD) -o ex_commandline $(EX_COMMANDLINE) $(LIBS)

TPTPSIG = tptp-sig.o ../lib/CONTROL.a ../lib/HEURISTICS.a \
				../lib/CLAUSES.a ../lib/ORDERINGS.a \
				../lib/TERMS.a ../lib/INOUT.a ../lib/BASICS.a

tptp-sig: $(TPTPSIG)
	$(LD) -o tptp-sig $(TPTPSIG) $(LIBS)

include Makefile.dependencies


