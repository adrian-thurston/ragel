SUBDIRS = c dot xml go

# d crack cs java ml rbx ruby

COLM = @COLM_FULL@

bin_PROGRAMS = ragel

ragel_CPPFLAGS = -I$(top_srcdir)/aapl

ragel_SOURCES = \
	buffer.h inputdata.h redfsm.h parsedata.h \
	dotcodegen.h parsetree.h rlscan.h version.h common.h \
	fsmgraph.h pcheck.h gendata.h ragel.h load.h if.h \
	main.cc parsetree.cc parsedata.cc fsmstate.cc fsmbase.cc \
	fsmattach.cc fsmmin.cc fsmgraph.cc fsmap.cc fsmcond.cc \
	inputdata.cc common.cc redfsm.cc gendata.cc allocgen.cc \
	load.cc parse.c if.cc

ragel_CXXFLAGS = -Wall

ragel_LDADD = \
	c/libc.a \
	dot/libdot.a \
	xml/libxml.a \
	go/libgo.a

#	crack/libcrack.a
#	cs/libcs.a
#	d/libd.a
#	java/libjava.a
#	ml/libml.a
#	rbx/librbx.a
#	ruby/libruby.a

BUILT_SOURCES = version.h

version.h: Makefile
	echo '#define VERSION "$(PACKAGE_VERSION)"' > version.h
	echo '#define PUBDATE "$(PUBDATE)"' >> version.h

EXTRA_DIST = ragel.lm rlparse.lm

CLEANFILES = parse.c if.h if.cc

parse.c: ragel.lm rlparse.lm
	$(COLM) -c -o parse.c -e if.h -x if.cc rlparse.lm
if.h: parse.c
if.cc: parse.c

ragel-if.$(OBJEXT): if.h if.cc parse.c
ragel-load.$(OBJEXT): if.h if.cc parse.c
