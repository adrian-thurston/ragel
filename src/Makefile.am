SUBDIRS = c dot xml

# d crack cs go java ml rbx ruby

bin_PROGRAMS = ragel

ragel_CPPFLAGS = -I$(top_srcdir)/aapl

ragel_SOURCES = \
	buffer.h inputdata.h redfsm.h parsedata.h rlparse.h \
	dotcodegen.h parsetree.h rlscan.h version.h common.h \
	fsmgraph.h pcheck.h gendata.h ragel.h \
	main.cc parsetree.cc parsedata.cc fsmstate.cc fsmbase.cc \
	fsmattach.cc fsmmin.cc fsmgraph.cc fsmap.cc fsmcond.cc rlscan.cc rlparse.cc \
	inputdata.cc common.cc redfsm.cc gendata.cc allocgen.cc

ragel_CXXFLAGS = -Wall

ragel_LDADD = \
	c/libc.a \
	dot/libdot.a \
	xml/libxml.a

#	crack/libcrack.a
#	cs/libcs.a
#	d/libd.a
#	go/libgo.a
#	java/libjava.a
#	ml/libml.a
#	rbx/librbx.a
#	ruby/libruby.a

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
