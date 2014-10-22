COLM = @COLM_FULL@

bin_PROGRAMS = ragel rlhc

ragel_CPPFLAGS = -I$(top_srcdir)/aapl -DBINDIR='"@bindir@"'

ragel_SOURCES = \
	buffer.h inputdata.h redfsm.h parsedata.h \
	parsetree.h rlscan.h version.h common.h \
	fsmgraph.h pcheck.h gendata.h ragel.h load.h if.h \
	codegen.h binary.h binloop.h binexp.h flat.h flatloop.h \
	flatexp.h goto.h gotoloop.h gotoexp.h ipgoto.h xml.h dot.h \
	binbasic.h \
	\
	main.cc parsetree.cc parsedata.cc fsmstate.cc fsmbase.cc \
	fsmattach.cc fsmmin.cc fsmgraph.cc fsmap.cc fsmcond.cc \
	inputdata.cc common.cc redfsm.cc gendata.cc allocgen.cc \
	load.cc parse.c if.cc codegen.cc binary.cc binloop.cc \
	binexp.cc flat.cc flatloop.cc flatexp.cc goto.cc \
	gotoloop.cc gotoexp.cc ipgoto.cc xml.cc dot.cc binbasic.cc

ragel_CXXFLAGS = -Wall

BUILT_SOURCES = version.h

version.h: Makefile
	echo '#define VERSION "$(PACKAGE_VERSION)"' > version.h
	echo '#define PUBDATE "$(PUBDATE)"' >> version.h

EXTRA_DIST = \
	ragel.lm \
	rlhc.lm \
	rlhc-c.lm \
	rlhc-d.lm \
	rlhc-cs.lm \
	rlhc-go.lm \
	rlhc-ruby.lm \
	rlhc-java.lm \
	rlhc-ml.lm \
	rlparse.lm

CLEANFILES = parse.c if.h if.cc

parse.c: ragel.lm rlparse.lm
	$(COLM) -c -o parse.c -e if.h -x if.cc rlparse.lm
if.h: parse.c
if.cc: parse.c

ragel-if.$(OBJEXT): if.h if.cc parse.c
ragel-load.$(OBJEXT): if.h if.cc parse.c

RLHC = \
	rlhc-c.lm \
	rlhc-d.lm \
	rlhc-go.lm \
	rlhc-cs.lm \
	rlhc-java.lm \
	rlhc-ruby.lm \
	rlhc-ml.lm

rlhc$(EXEEXT): rlhc.lm $(RLHC)
	$(COLM) -o $@ $<
