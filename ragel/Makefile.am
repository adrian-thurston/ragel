
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
	csgoto.h gendata.h ragel.h rubyfflat.h goipgoto.h \
	main.cpp parsetree.cpp parsedata.cpp fsmstate.cpp fsmbase.cpp \
	fsmattach.cpp fsmmin.cpp fsmgraph.cpp fsmap.cpp rlscan.cpp rlparse.cpp \
	inputdata.cpp common.cpp redfsm.cpp gendata.cpp cdcodegen.cpp \
	cdtable.cpp cdftable.cpp cdflat.cpp cdfflat.cpp cdgoto.cpp cdfgoto.cpp \
	cdipgoto.cpp cdsplit.cpp javacodegen.cpp rubycodegen.cpp rubytable.cpp \
	rubyftable.cpp rubyflat.cpp rubyfflat.cpp rbxgoto.cpp cscodegen.cpp \
	cstable.cpp csftable.cpp csflat.cpp csfflat.cpp csgoto.cpp csfgoto.cpp \
	csipgoto.cpp cssplit.cpp dotcodegen.cpp xmlcodegen.cpp goipgoto.cpp

BUILT_SOURCES = \
	rlscan.cpp rlparse.h rlparse.cpp version.h

version.h: Makefile
	echo '#define VERSION "$(PACKAGE_VERSION)"' > version.h
	echo '#define PUBDATE "$(PUBDATE)"' >> version.h

EXTRA_DIST = rlscan.rl rlparse.kh rlparse.kl

if BUILD_PARSERS

CLEANFILES = \
	rlscan.cpp rlparse.h rlparse.cpp

rlparse.h: rlparse.kh
	kelbt -o $@ $<

rlparse.cpp: rlparse.kl rlparse.kh
	kelbt -o $@ $<

# This dependency comes from the import of the parser defines
# into the scanner.
rlscan.cpp: rlparse.h

rlscan.cpp: rlscan.rl
	ragel -G2 -I$(builddir) -o $@ $<

endif
