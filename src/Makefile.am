COLM = @COLM@

bin_PROGRAMS = ragel rlhc

ragel_CPPFLAGS = -I$(top_srcdir)/aapl -DBINDIR='"@bindir@"'

ragel_SOURCES = \
	buffer.h inputdata.h redfsm.h parsedata.h \
	parsetree.h rlscan.h common.h \
	fsmgraph.h pcheck.h gendata.h ragel.h load.h if.h codegen.h \
	binary.h bingotoloop.h bingotoexp.h binvarloop.h \
	flat.h flatgotoloop.h flatgotoexp.h \
	goto.h switchgotoloop.h switchgotoexp.h \
	ipgoto.h xml.h dot.h \
	\
	main.cc parsetree.cc parsedata.cc fsmstate.cc fsmbase.cc \
	fsmattach.cc fsmmin.cc fsmgraph.cc fsmap.cc fsmcond.cc \
	inputdata.cc common.cc redfsm.cc gendata.cc allocgen.cc \
	load.cc parse.c if.cc codegen.cc \
	binary.cc bingotoloop.cc bingotoexp.cc binvarloop.cc \
	flat.cc flatgotoloop.cc flatgotoexp.cc \
	goto.cc switchgotoloop.cc switchgotoexp.cc \
	ipgoto.cc xml.cc dot.cc 

nodist_ragel_SOURCES = version.h

ragel_LDADD = -lcolmd

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

rlhc_SOURCES =

rlhc$(EXEEXT): rlhc.lm $(RLHC)
	$(COLM) -o $@ $<
