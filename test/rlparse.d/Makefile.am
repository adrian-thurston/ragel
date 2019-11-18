COLM_xCPPFLAGS = -I../../colm/include
COLM_xLDFLAGS = -L../../colm

noinst_PROGRAMS = rlparse

rlparse_SOURCES = parse.c if.h if.cc commit.cc \
	actparams.cc parsetree.cc parsedata.cc \
	common.cc fsmap.cc fsmbase.cc idbase.cc \
	load.cc reducer.cc inputdata.cc fsmgraph.cc \
	main.cc dump.cc reducer.h vector.h inputdata.h \
	table.h gendata.h common.h ragel.h parsedata.h \
	action.h parsetree.h redfsm.h fsmgraph.h pcheck.h \
	load.h version.h config.h buffer.h \
	avlbasic.h avlcommon.h avlibasic.h avlikeyless.h \
	avlimap.h avlimel.h avlimelkey.h avliset.h avlitree.h \
	avlkeyless.h avlmap.h avlmel.h avlmelkey.h avlset.h \
	avltree.h compare.h bstcommon.h bstmap.h bstset.h \
	bsttable.h dlcommon.h dlist.h dlistmel.h dlistval.h \
	bubblesort.h mergesort.h sbstmap.h sbstset.h sbsttable.h \
	svector.h

rlparse_CPPFLAGS = $(COLM_xCPPFLAGS)
rlparse_LDADD = -lcolm
rlparse_LDFLAGS = $(COLM_xLDFLAGS)

EXTRA_DIST = \
	reducer.lm rlparse.lm ragel.lm \
	ragel-c.lm ragel-crack.lm ragel-ocaml.lm \
	ragel-ruby.lm gentests \
	case/rlparse.h case/rlscan.rl \
	case/rlscan--colm-frontend.exp case/rlscan--reduce-frontend.exp

BUILT_SOURCES = parse.c if.h if.cc commit.cc

parse.c: rlparse.lm reducer.lm $(RAGEL_LM) $(COLM)
	$(COLM) -c -b rlparse_object -o $@ -e if.h -x if.cc -m commit.cc $<

if.h: parse.c
if.cc: parse.c
commit.cc: parse.c

