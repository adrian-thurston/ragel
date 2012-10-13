SUBDIRS = c d crack cs dot go java ml rbx ruby xml

INCLUDES = -I$(top_srcdir)/aapl

bin_PROGRAMS = ragel

ragel_CXXFLAGS = -Wall

ragel_SOURCES = \
	buffer.h cdgoto.h cscodegen.h csipgoto.h inputdata.h rbxgoto.h \
	rubyflat.h cdcodegen.h cdipgoto.h csfflat.h cssplit.h javacodegen.h \
	redfsm.h rubyftable.h cdfflat.h cdsplit.h csfgoto.h cstable.h \
	parsedata.h rlparse.h rubytable.h cdfgoto.h cdtable.h csflat.h \
	dotcodegen.h parsetree.h rlscan.h version.h cdflat.h common.h \
	csftable.h fsmgraph.h pcheck.h rubycodegen.h xmlcodegen.h cdftable.h \
	csgoto.h gendata.h ragel.h rubyfflat.h \
	gocodegen.h gotable.h goftable.h goflat.h gofflat.h gogoto.h gofgoto.h \
	goipgoto.h gotablish.h \
	mlcodegen.h mltable.h mlftable.h mlflat.h mlfflat.h mlgoto.h mlfgoto.h \
	main.cc parsetree.cc parsedata.cc fsmstate.cc fsmbase.cc \
	fsmattach.cc fsmmin.cc fsmgraph.cc fsmap.cc fsmcond.cc rlscan.cc rlparse.cc \
	inputdata.cc common.cc redfsm.cc gendata.cc cdcodegen.cc \
	cdtable.cc cdftable.cc cdflat.cc cdfflat.cc cdgoto.cc cdfgoto.cc \
	cdipgoto.cc cdsplit.cc javacodegen.cc rubycodegen.cc rubytable.cc \
	rubyftable.cc rubyflat.cc rubyfflat.cc rbxgoto.cc cscodegen.cc \
	cstable.cc csftable.cc csflat.cc csfflat.cc csgoto.cc csfgoto.cc \
	csipgoto.cc cssplit.cc dotcodegen.cc dotcodegen-orig.cc xmlcodegen.cc reducedgen.cc \
	gocodegen.cc gotable.cc goftable.cc goflat.cc gofflat.cc gogoto.cc gofgoto.cc \
	goipgoto.cc gotablish.cc \
	mlcodegen.cc mltable.cc mlftable.cc mlflat.cc mlfflat.cc mlgoto.cc mlfgoto.cc \
	crackcodegen.h crackcodegen.cc crackflat.h crackflat.cc

ragel_LDADD = \
	c/libc.a \
	crack/libcrack.a \
	cs/libcs.a \
	d/libd.a \
	dot/libdot.a \
	go/libgo.a \
	java/libjava.a \
	ml/libml.a \
	rbx/librbx.a \
	ruby/libruby.a \
	xml/libxml.a

BUILT_SOURCES = \
	rlscan.cc rlparse.h rlparse.cc version.h

version.h: Makefile
	echo '#define VERSION "$(PACKAGE_VERSION)"' > version.h
	echo '#define PUBDATE "$(PUBDATE)"' >> version.h

EXTRA_DIST = rlscan.rl rlparse.kh rlparse.kl

if BUILD_PARSERS

CLEANFILES = \
	rlscan.cc rlparse.h rlparse.cc

rlparse.h: rlparse.kh
	kelbt -o $@ $<

rlparse.cc: rlparse.kl rlparse.kh
	kelbt -o $@ $<

# This dependency comes from the import of the parser defines
# into the scanner.
rlscan.cc: rlparse.h

rlscan.cc: rlscan.rl
	ragel -G2 -I$(builddir) -o $@ $<

endif
