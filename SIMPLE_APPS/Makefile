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

PROJECT = ex_commandline term2dag enigma-features #enigmatic-features #enigma-problem-features #enigma-svd-traintrans
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

ENIGMACONJFEATURES = enigma-features.o \
            ../lib/CONTROL.a ../lib/PCL2.a ../lib/CLAUSES.a ../lib/ORDERINGS.a \
            ../lib/TERMS.a ../lib/INOUT.a ../lib/BASICS.a ../lib/HEURISTICS.a

enigma-features: $(ENIGMACONJFEATURES)
	$(LD) -o enigma-features $(ENIGMACONJFEATURES) $(LIBS)

ENIGMATICFEATURES = enigmatic-features.o \
				../lib/CONTROL.a ../lib/PCL2.a ../lib/CLAUSES.a ../lib/ORDERINGS.a \
				../lib/TERMS.a ../lib/INOUT.a ../lib/BASICS.a ../lib/HEURISTICS.a

enigmatic-features: $(ENIGMATICFEATURES)
	$(LD) -o enigmatic-features $(ENIGMATICFEATURES) $(LIBS)

ENIGMASVDTRAINTRANS = enigma-svd-traintrans.o \
				../lib/HEURISTICS.a ../lib/INOUT.a ../lib/BASICS.a \
            ../lib/CLAUSES.a ../lib/ORDERINGS.a ../lib/TERMS.a \
				../CONTRIB/svdlibc/libsvd.a 

enigma-svd-traintrans: $(ENIGMASVDTRAINTRANS)
	$(LD) -o enigma-svd-traintrans $(ENIGMASVDTRAINTRANS) $(LIBS)

ENIGMAPROBLEMFEATURES = enigma-problem-features.o \
            ../lib/PCL2.a ../lib/CLAUSES.a ../lib/ORDERINGS.a \
            ../lib/TERMS.a ../lib/INOUT.a ../lib/BASICS.a ../lib/HEURISTICS.a

enigma-problem-features: $(ENIGMAPROBLEMFEATURES)
	$(LD) -o enigma-problem-features $(ENIGMAPROBLEMFEATURES) $(LIBS)


include Makefile.dependencies


